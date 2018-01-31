#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Simulation
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/GotthardControl.o \
	${OBJECTDIR}/src/JungfrauControl.o \
	${OBJECTDIR}/src/SlsControl.o

# Test Directory
TESTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}/tests

# Test Files
TESTFILES= \
	${TESTDIR}/TestFiles/f1

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L${KARABO}/lib -L${KARABO}/extern/lib -Wl,-rpath,${KARABO}/lib -Wl,-rpath,${KARABO}/extern/lib -lSlsSimulation -lkarabo `pkg-config --libs karaboDependencies`  

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libslsControl.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libslsControl.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libslsControl.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} -shared -fPIC

${OBJECTDIR}/src/GotthardControl.o: src/GotthardControl.cc 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -DSLS_SIMULATION -I${KARABO}/extern/include -I${KARABO}/include `pkg-config --cflags karaboDependencies` -std=c++11  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/GotthardControl.o src/GotthardControl.cc

${OBJECTDIR}/src/JungfrauControl.o: src/JungfrauControl.cc 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -DSLS_SIMULATION -I${KARABO}/extern/include -I${KARABO}/include `pkg-config --cflags karaboDependencies` -std=c++11  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/JungfrauControl.o src/JungfrauControl.cc

${OBJECTDIR}/src/SlsControl.o: src/SlsControl.cc 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -DSLS_SIMULATION -I${KARABO}/extern/include -I${KARABO}/include `pkg-config --cflags karaboDependencies` -std=c++11  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/SlsControl.o src/SlsControl.cc

# Subprojects
.build-subprojects:

# Build Test Targets
.build-tests-conf: .build-conf ${TESTFILES}
${TESTDIR}/TestFiles/f1: ${TESTDIR}/src/tests/SlsControlTest.o ${TESTDIR}/src/tests/test_runner.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc}   -o ${TESTDIR}/TestFiles/f1 $^ ${LDLIBSOPTIONS} `cppunit-config --libs`   


${TESTDIR}/src/tests/SlsControlTest.o: src/tests/SlsControlTest.cc 
	${MKDIR} -p ${TESTDIR}/src/tests
	${RM} "$@.d"
	$(COMPILE.cc) -g -DSLS_SIMULATION -I${KARABO}/extern/include -I${KARABO}/include `pkg-config --cflags karaboDependencies` -std=c++11 `cppunit-config --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/src/tests/SlsControlTest.o src/tests/SlsControlTest.cc


${TESTDIR}/src/tests/test_runner.o: src/tests/test_runner.cc 
	${MKDIR} -p ${TESTDIR}/src/tests
	${RM} "$@.d"
	$(COMPILE.cc) -g -DSLS_SIMULATION -I${KARABO}/extern/include -I${KARABO}/include `pkg-config --cflags karaboDependencies` -std=c++11 `cppunit-config --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/src/tests/test_runner.o src/tests/test_runner.cc


${OBJECTDIR}/src/GotthardControl_nomain.o: ${OBJECTDIR}/src/GotthardControl.o src/GotthardControl.cc 
	${MKDIR} -p ${OBJECTDIR}/src
	@NMOUTPUT=`${NM} ${OBJECTDIR}/src/GotthardControl.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -g -DSLS_SIMULATION -I${KARABO}/extern/include -I${KARABO}/include `pkg-config --cflags karaboDependencies` -std=c++11  -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/GotthardControl_nomain.o src/GotthardControl.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/src/GotthardControl.o ${OBJECTDIR}/src/GotthardControl_nomain.o;\
	fi

${OBJECTDIR}/src/JungfrauControl_nomain.o: ${OBJECTDIR}/src/JungfrauControl.o src/JungfrauControl.cc 
	${MKDIR} -p ${OBJECTDIR}/src
	@NMOUTPUT=`${NM} ${OBJECTDIR}/src/JungfrauControl.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -g -DSLS_SIMULATION -I${KARABO}/extern/include -I${KARABO}/include `pkg-config --cflags karaboDependencies` -std=c++11  -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/JungfrauControl_nomain.o src/JungfrauControl.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/src/JungfrauControl.o ${OBJECTDIR}/src/JungfrauControl_nomain.o;\
	fi

${OBJECTDIR}/src/SlsControl_nomain.o: ${OBJECTDIR}/src/SlsControl.o src/SlsControl.cc 
	${MKDIR} -p ${OBJECTDIR}/src
	@NMOUTPUT=`${NM} ${OBJECTDIR}/src/SlsControl.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -g -DSLS_SIMULATION -I${KARABO}/extern/include -I${KARABO}/include `pkg-config --cflags karaboDependencies` -std=c++11  -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/SlsControl_nomain.o src/SlsControl.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/src/SlsControl.o ${OBJECTDIR}/src/SlsControl_nomain.o;\
	fi

# Run Test Targets
.test-conf:
	@if [ "${TEST}" = "" ]; \
	then  \
	    ${TESTDIR}/TestFiles/f1 || true; \
	else  \
	    ./${TEST} || true; \
	fi

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libslsControl.${CND_DLIB_EXT}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
