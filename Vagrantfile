# -*- mode: ruby -*-
# vi: set ft=ruby :

$script = <<SCRIPT
apt-get install -y python-software-properties software-properties-common python-virtualenv python-dev python-pip
apt-get install -y build-essential mingw-w64 zlib1g-dev git cmake 
pip install -U setuptools
pip install virtualenv
SCRIPT

VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|

  config.vm.box = "phusion-open-ubuntu-14.04-amd64"
  config.vm.box_url = "https://oss-binaries.phusionpassenger.com/vagrant/boxes/latest/ubuntu-14.04-amd64-vbox.box"

  # note that, with these changes, the private key is now on the box.  This is necessary to make testing
  # happen smoothly, but should we ever try to copy virtual machines around (a la Docker), then we need to make
  # sure to wipe the creds first.
  config.vm.provision "file", source: "~/.ssh/id_rsa", destination: "~/.ssh/id_rsa"
  config.vm.provision "file", source: "~/.ssh/id_rsa.pub", destination: "~/.ssh/id_rsa.pub"
  config.vm.provision "file", source: "~/.ssh/known_hosts", destination: "~/.ssh/known_hosts"

  config.vm.provision "shell", inline: $script
  if Vagrant.has_plugin?("vagrant-cachier")
    config.cache.scope = :box
  end

end
