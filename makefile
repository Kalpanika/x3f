X3F_TEST_FILES_REPO=https://github.com/Kalpanika/x3f_test_files.git
X3F_TEST_FILES_COMMIT=3ffc10d0a65f14b53f34c979d2327673677748f7
X3F_TEST_FILES=x3f_test_files
VENV=venv
REQUIREMENTS=requirements.txt
VIRTUALENV=virtualenv
BEHAVE=venv/bin/behave

# Set the SYS variable
include sys.mk

.PHONY: default all dist clean clobber clean_opencv

default: all

all dist clean clobber:
	$(MAKE) -C src $@

ifeq ($(HOST), linux-x86_64)
.PHONY: dist-all dist-osx dist-32 dist-64

dist-all: dist-osx dist-32 dist-64

dist-osx:
	$(MAKE) TARGET=osx-universal dist

dist-32:
	$(MAKE) TARGET=windows-i686 dist

dist-64:
	$(MAKE) TARGET=linux-x86_64 dist
	$(MAKE) TARGET=windows-x86_64 dist
endif

ifeq ($(TARGET), osx-universal)

all dist: deps/lib/osx-x86_64/opencv/.success deps/lib/osx-i386/opencv/.success

deps/lib/osx-x86_64/opencv/.success:
	$(MAKE) TARGET=osx-x86_64 $@

deps/lib/osx-i386/opencv/.success:
	$(MAKE) TARGET=osx-i386 $@

else

OPENCV = deps/lib/$(TARGET)/opencv

all dist: | $(OPENCV)/.success

$(OPENCV)/.success:
	./install_opencv.sh $(TARGET) $(CMAKE_TOOLCHAIN)

endif

clean_opencv:
	-@rm -rf deps/lib/*/opencv


ifeq ($(TARGET_SYS), windows)
EXE = .exe
else
EXE =
endif

.PHONY: check check_deps test_files clean_deps

check_deps: $(VENV)/.setup.touch test_files

$(VENV):
	virtualenv $@

$(VENV)/.setup.touch: $(REQUIREMENTS) | $(VENV)
	$(VENV)/bin/pip install -r $< && touch $@

check: check_deps dist
	DIST_LOC=dist/x3f_tools-$(shell git describe --always --dirty --tag)-$(TARGET)/bin/x3f_extract$(EXE) $(BEHAVE)

clean_deps:
	rm -rf $(VENV)
	rm -rf $(X3F_TEST_FILES)

test_files:
	@if test -d $(X3F_TEST_FILES); then echo "Test files alread cloned"; else git clone $(X3F_TEST_FILES_REPO); fi
	@if test -e $(X3F_TEST_FILES)/$(X3F_TEST_FILES_COMMIT).tfile; then echo "Already pulled to commit $(X3F_TEST_FILES_COMMIT)"; else cd $(X3F_TEST_FILES) && if test -e *.tfile; then rm *.tfile; fi && git fetch && git checkout $(X3F_TEST_FILES_COMMIT) && cd .. && touch $(X3F_TEST_FILES)/$(X3F_TEST_FILES_COMMIT).tfile; fi
