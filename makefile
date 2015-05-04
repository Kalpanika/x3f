X3F_TEST_FILES_REPO=git@github.com:Kalpanika/x3f_test_files.git
X3F_TEST_FILES_COMMIT=fb0d6ea0ee17b1e50652dc9a961759a04449121a
VENV?=venv
VIRTUALENVDIR?=$(CURDIR)/$(VENV)
REQUIREMENTS?=$(CURDIR)/requirements.txt
VIRTUALENV=virtualenv
BEHAVE=venv/bin/behave

# Set the SYS variable
include sys.mk

.PHONY: default all dist clean clobber clean_opencv check clean_deps

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

check_deps:  $(VIRTUALENVDIR) $(VIRTUALENVDIR)/.setup.touch test_files
	@hash $(VIRTUALENV) || echo "$(VIRTUALENV) is not installed in the path, please install it."

$(VIRTUALENVDIR):
	virtualenv $(VENV)

$(VIRTUALENVDIR)/.setup.touch: $(REQUIREMENTS) | $(VENV)
	$(VENV)/bin/pip install -r $< && touch $@

check: check_deps dist
	$(BEHAVE)

clean_deps:
	rm -rf $(VIRTUALENVDIR)

test_files:
	@if test -d x3f_test_files; then echo "Test files alread cloned"; else git clone $(X3F_TEST_FILES_REPO); fi
	@cd x3f_test_files && git fetch && git checkout $(X3F_TEST_FILES_COMMIT)
