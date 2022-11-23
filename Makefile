SUBDIRS = slsDetectorsSimulation slsControl slsReceiver

.PHONY: build package test $(SUBDIRS)

CONF ?= Debug
DISTDIR = dist/$(CONF)/cmake

ifndef KARABO
	KARABO := $(shell cat ${HOME}/.karabo/karaboFramework)
endif

build: slsDetectorsSimulation
	$(MAKE) -C slsControl build
	$(MAKE) -C slsReceiver build
	mkdir -p $(DISTDIR)
	cp -a slsControl/$(DISTDIR)/*.so $(DISTDIR)
	cp -a slsReceiver/$(DISTDIR)/*.so $(DISTDIR)

package: build
	@$(KARABO)/bin/.bundle-cppplugin.sh dist $(CONF) GNU-Linux-x86

test: slsDetectorsSimulation
	$(MAKE) -C slsControl test
	$(MAKE) -C slsReceiver test

clean:
	$(MAKE) -C slsControl clean
	$(MAKE) -C slsReceiver clean
	$(MAKE) -C slsDetectorsSimulation clean
	rm -f $(DISTDIR)/*.so

slsDetectorsSimulation:
ifeq ($(CONF),Simulation)
	$(MAKE) -C $@ CONF=Release
endif

slsControl: slsDetectorsSimulation
	$(MAKE) -C $@

slsReceiver: slsDetectorsSimulation
	$(MAKE) -C $@

PACKAGE_NAME=$(shell basename -s .git `git remote -v | grep fetch | head -n1 | awk '{ print $$2 }' `)
