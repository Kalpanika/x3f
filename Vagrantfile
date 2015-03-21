# -*- mode: ruby -*-
# vi: set ft=ruby :

$script = <<SCRIPT
apt-get install -y build-essential
apt-get install -y zlib1g-dev
apt-get install -y git
apt-get install -y cmake
SCRIPT

VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|

  config.vm.box = "phusion-open-ubuntu-14.04-amd64"
  config.vm.box_url = "https://oss-binaries.phusionpassenger.com/vagrant/boxes/latest/ubuntu-14.04-amd64-vbox.box"

  config.vm.provision "shell", inline: $script
  if Vagrant.has_plugin?("vagrant-cachier")
    config.cache.scope = :box
  end

end
