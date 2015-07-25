# -*- mode: ruby -*-
# vi: set ft=ruby :

$script = <<SCRIPT
sudo DEBIAN_FRONTEND=noninteractive apt-get -y -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" update
sudo DEBIAN_FRONTEND=noninteractive apt-get -y -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" upgrade
apt-get install -y python-software-properties software-properties-common python-virtualenv python-dev python-pip
apt-get install -y build-essential mingw-w64 zlib1g-dev git cmake zip ntp
pip install -U setuptools
pip install virtualenv
SCRIPT

VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|

  config.vm.provider :virtualbox do |vb|
    config.vm.box = "ubuntu/trusty64"
    vb.memory = 2048
    vb.cpus = 2
  end

  config.vm.provision "shell", inline: $script
  if Vagrant.has_plugin?("vagrant-cachier")
    config.cache.scope = :box
  end

end
