#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <handler.h>
#include "multilist.h"
#include "ooo.h"
#include "../stcp_utils.h"

#define LIKELY(x)       __builtin_expect((x),1)
#define UNLIKELY(x)     __builtin_expect((x),0)

#define GET_NODE_PTR(list, idx) ((multilist_node_t*) (((uint8_t*)list) + sizeof(multilist_t) + sizeof(multilist_node_t) * idx))

void _compaction(multilist_t *mlist);
void _merge_list(multilist_t *mlist, uint32_t list_id);

// Finds the node in the list
uint32_t multilist_find_pred(multilist_t *list, uint32_t list_id, multilist_range_t range, multilist_node_t **pred_node, multilist_node_t **curr_node)
{
    multilist_node_t *curr = list->list_head[list_id];
    multilist_node_t *pred = NULL;

    // this works without checking EOL because of the sentinels
    while (curr != NULL && curr->range.right < range.left)
    {
        pred = curr;
        curr = curr->next;
    }

    S_ASSERT(curr==NULL || curr->range.left > range.right);

    *pred_node = pred;
    *curr_node = curr;

    return 1;
}

// push range in the OOO list identified by list_id
uint32_t multilist_range_push(multilist_t *mlist, uint32_t list_id, multilist_range_t range)
{
    multilist_node_t *pred_node, *curr_node, *to_delete=NULL;

    spin_lock_lock(&(mlist->list_lock[list_id]));

    multilist_find_pred(mlist, list_id, range, &pred_node, &curr_node);

    // check if we have to merge
    bool connects_left = pred_node != NULL && pred_node->range.right + 1 == range.left;
    bool connects_right = curr_node != NULL && range.right + 1 == curr_node->range.left;

    if (connects_left && connects_right)
    {
        pred_node->range.right = curr_node->range.right;

        // remove curr
        pred_node->next = curr_node->next;
        hpool_push(&(mlist->free_pool), list_id, (uint8_t*) curr_node);
    }
    else if (connects_left)
    {
        pred_node->range.right = range.right;
    }
    else if (connects_right)
    {
        curr_node->range.left = range.left;
    }
    else
    {
        multilist_node_t *new_node = NULL;
        uint32_t got_free_node = hpool_pop(&(mlist->free_pool), list_id, (uint8_t **) &new_node);
        if (got_free_node != HPOOL_SUCCESS)
        {
            spin_lock_unlock(&(mlist->list_lock[list_id]));
            return OOO_NO_SPACE;
        }
        else
        {
            //printf("inserting between %ld and %ld\n", pred_node->idx, curr_node->idx);
            new_node->range = range;
            new_node->next = curr_node;
            
            multilist_node_t **link_ptr = (pred_node!=NULL) ? &(pred_node->next) : &(mlist->list_head[list_id]);
            *link_ptr = new_node;
        }
    }
    
    spin_lock_unlock(&(mlist->list_lock[list_id]));

    return OOO_SUCCESS;
}

/*
uint32_t multilist_front_flush(multilist_t *mlist, uint32_t left, uint32_t max_iters)
{   
    uint32_t flushed_bytes = 0;
    for (int iter=0; iter<max_iters; iter++)
    {
        uint32_t found_one = 0;
        for (int i=0; i<NUM_LISTS; i++)
        {
            if (mlist->list_head[i] == NULL) continue;

            // check if HPU i has the next seqno
            if (mlist->list_head[i]->range.left == left)
            {
                found_one = 1;

                // lock list
                spin_lock_lock(&(mlist->list_lock[i]));
                
                uint32_t segment_size = (mlist->list_head[i]->range.right - mlist->list_head[i]->range.left) + 1;
                flushed_bytes += segment_size;
                left += segment_size;

                multilist_node_t *list_head = mlist->list_head[i];

                //remove node
                mlist->list_head[i] = list_head->next;

                // unlock list
                spin_lock_unlock(&(mlist->list_lock[i]));
                
                //release node to the pool
                hpool_push(&(mlist->free_pool), i, (uint8_t*) list_head);   
            }
        } 

        if (!found_one) break;
    }

    return flushed_bytes;
}
*/

uint32_t multilist_front_flush(multilist_t *mlist, uint32_t list_id, uint32_t left, uint32_t max_iters)
{
    _compaction(mlist);

    multilist_node_t *head = mlist->compact_list_head;

    if (head==NULL || head->range.left != left) return 0;

    uint32_t bytes = head->range.right - head->range.left + 1;
    mlist->compact_list_head = head->next;
    hpool_push(&(mlist->free_pool), list_id, (uint8_t*) head);   

    return bytes;
}

void _compaction(multilist_t *mlist)
{
    for (int i=0; i<NUM_LISTS; i++)
    {
        spin_lock_lock(&(mlist->list_lock[i]));
        _merge_list(mlist, i);
        spin_lock_unlock(&(mlist->list_lock[i]));
    }
}

void _merge_list(multilist_t *mlist, uint32_t list_id)
{
    multilist_node_t* list1 = mlist->compact_list_head;
    multilist_node_t* list2 = mlist->list_head[list_id];

    multilist_node_t* pred_list1 = NULL;

    while (list2!=NULL)
    {
         multilist_node_t* list2_next_tmp = list2->next;

        // remove from "victim" list
        mlist->list_head[list_id] = list2->next;

        // get to the right position in list1
        while (LIKELY(list1!=NULL && list2->range.left >= list1->range.left))
        {
            pred_list1 = list1;
            list1 = list1->next;
        }

        // check how this node connects
        bool connects_left = pred_list1 != NULL && pred_list1->range.right + 1 == list2->range.left;
        bool connects_right = list1 != NULL && list2->range.right + 1 == list1->range.left;
        
        if (connects_left && connects_right)
        {
            pred_list1->range.right = list1->range.right;

            // remove merged node in shared list
            pred_list1->next = list1->next;
            
            hpool_push(&(mlist->free_pool), list_id, (uint8_t*) list1);

            // we are removing the current list1 node, so update list1 ptr
            list1 = pred_list1->next;
        }
        else if (connects_left)
        {
            pred_list1->range.right = list2->range.right;
        }
        else if (connects_right)
        {
            list1->range.left = list2->range.left;
        }
        else
        {
            list2->next = list1;

            multilist_node_t **list1_link_ptr = (pred_list1!=NULL) ? &(pred_list1->next) : &(mlist->compact_list_head);
            *list1_link_ptr = list2;

            // we now have a new pred_list1, which is the just inserted node
            pred_list1 = list2; 
        }

        if (connects_left || connects_right)
        {
            // remove node from list2
            hpool_push(&(mlist->free_pool), list_id, (uint8_t*) list2);
        }

        list2 = list2_next_tmp;
    }
}
