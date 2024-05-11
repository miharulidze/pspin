import numpy as np


def jain(x):
    """Implements Jain's fairness index on an array of values.

    Arguments:
        x: An array of values for which the Jain's index is meant to be computed.

    Returns:
        The Jain's index.
    """
    if np.sum(x**2) == 0:
        return 1
    return np.sum(x)**2/(np.sum(x**2)*len(x))


def linear_product_based(x):
    """Implements linear product based fairness index on an array of values.

    Arguments:
        x: An array of values for which the index is meant to be computed.

    Returns:
        The linear product based index.
    """
    return bossaer_product_based(x, 1)


def g_product_based(x, k):
    """Implements G's product based fairness index on an array of values.

    Arguments:
        x: An array of values for which the index is meant to be computed.
        k: Order of the index.

    Returns:
        The G's fairness index.
    """
    return np.product(np.sin(np.pi*x/(2*np.max(x)))**(1/k))


def bossaer_product_based(x, k):
    """Implements Bossaer's product based fairness index on an array of values.

    Arguments:
        x: An array of values for which the index is meant to be computed.
        k: Order of the index.

    Returns:
        The G's fairness index.
    """
    if len(x) == 0 or np.max(x) == 0:
        return 1
    return np.product((x/np.max(x))**(1/k))


if __name__=="__main__":
    print(bossaer_product_based(np.array([100.0, 10.0, 5.0, 1.0, 2.0]), 10))
