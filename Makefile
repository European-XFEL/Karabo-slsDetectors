SUBDIRS = slsDetectorsSimulation slsControl slsReceiver

.PHONY: build package test $(SUBDIRS)

build: slsDetectorsSimulation
	$(MAKE) -C slsControl build
	$(MAKE) -C slsReceiver build

package: slsDetectorsSimulation
	$(MAKE) -C slsControl package
	$(MAKE) -C slsReceiver package

test: slsDetectorsSimulation
	$(MAKE) -C slsControl test
	$(MAKE) -C slsReceiver test

slsDetectorsSimulation:
ifeq ($(CONF),Simulation)
	$(MAKE) -C $@ CONF=Release
endif

slsControl: slsDetectorsSimulation
	$(MAKE) -C $@

slsReceiver: slsDetectorsSimulation
	$(MAKE) -C $@


