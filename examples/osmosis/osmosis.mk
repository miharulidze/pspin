all::
	make deploy
	make osmosis
	make osmosis_debug

../osmosis/osmosis_args.c: ../osmosis/osmosis_args.ggo
	gengetopt -i ../osmosis/osmosis_args.ggo -F osmosis_args --output-dir=../osmosis/

osmosis: driver/driver.c ../osmosis/osmosis_args.c ../osmosis/host_api/osmosis.c
	 gcc -std=c99 -I../osmosis/host_api/ -I../osmosis/handler_api/ -I$(PSPIN_RT)/runtime/include/ -I$(PSPIN_HW)/verilator_model/include driver/driver.c ../osmosis/host_api/osmosis.c ../osmosis/osmosis_args.c -L$(PSPIN_HW)/verilator_model/lib/ -lpspin -o sim_${SPIN_APP_NAME}

osmosis_debug: driver/driver.c ../osmosis/osmosis_args.c ../osmosis/host_api/osmosis.c
	 gcc -g -std=c99 -I../osmosis/host_api/ -I../osmosis/handler_api/ -I$(PSPIN_RT)/runtime/include/ -I$(PSPIN_HW)/verilator_model/include driver/driver.c ../osmosis/host_api/osmosis.c ../osmosis/osmosis_args.c -L$(PSPIN_HW)/verilator_model/lib/ -lpspin_debug -o sim_${SPIN_APP_NAME}_debug

clean::
	-@rm *.log 2>/dev/null || true
	-@rm -r build/ 2>/dev/null || true
	-@rm -r waves.vcd 2>/dev/null || true
	-@rm sim_${SPIN_APP_NAME} 2>/dev/null || true
	-@rm sim_${SPIN_APP_NAME}_debug 2>/dev/null || true

run::
	./sim_${SPIN_APP_NAME} | tee transcript

.PHONY: osmosis osmosis_debug clean run
