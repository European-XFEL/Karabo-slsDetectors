SUBDIRS = slsDetectorsSimulation slsControl slsReceiver

.PHONY: build package test $(SUBDIRS)

CONF ?= Debug
DISTDIR = dist/$(CONF)/GNU-Linux-x86

ifndef KARABO
	KARABO := $(shell cat ${HOME}/.karabo/karaboFramework)
endif

build: slsDetectorsSimulation
	$(MAKE) -C slsControl build
	$(MAKE) -C slsReceiver build

package: build
	mkdir -p $(DISTDIR)
	cp -a slsControl/$(DISTDIR)/*.so $(DISTDIR)
	cp -a slsReceiver/$(DISTDIR)/*.so $(DISTDIR)
	@$(KARABO)/bin/.bundle-cppplugin.sh dist $(CONF) GNU-Linux-x86

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


