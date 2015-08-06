This project contains tools for manipulating X3F files from Sigma cameras.
See doc/readme.txt and doc/copyright.txt.

-------
COMPILE
-------

Download the code.
run 'make'
<wait>
The bin directory contains binaries that can be run directly from the
command line.

-------------
DISTRIBUTIONS
-------------

Alternatively, you can create binaries for multiple platforms by calling:

    make dist-all

as well as the platform specific calls:

    make dist-osx
    make dist-32
    make dist-64

The distribution will be found in the file

    dist/x3f_tools-<VER>-<PLATFORM>

The <VER> part is either the git hash, or if there is a tag on current
commit, that tag.

-------
VAGRANT
-------

If you don't want to install all the dependencies, you can also
install and run in a vagrant.  Doing so requires downloading Vagrant
from http://www.vagrantup.com.  Once you've done so, the commands to
run the code are:

    vagrant up
    <wait>
    make

A note about cross-compilation in Vagrant:  We use the 'strip' command
on Windows binaries, but there is a [known issue using strip on virtualbox](https://www.virtualbox.org/ticket/8463).
We've found that running `make dist-64` multiple times resolves the
issue; that is, the cross compilation may take a few tries to run in
Vagrant, but it will work eventually.

-----
TESTS
-----

If you want to run tests, type:

    make check

The tests require that a python virtual environment be installable on
the current system.  That precursor can be met either by installing
`pip` and `virtualenv` on your local box, or by running through the
vagrant.  Once that requirement has been met, the makefile will ensure
that the other packages are installed properly.

The other requirement for the tests is to pull down sample images from
the x3f_test_files repository.  This is a one-time download of about
90 MB of Sigma images that are used to run tests.
