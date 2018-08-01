# -*- mode: ruby -*-
# vi: set ft=ruby :

$script = <<-SCRIPT
# apt
apt-get update
apt-get install -y build-essential libxmu-dev libxmu-headers libxpm-dev libmotif-dev imagemagick
SCRIPT

HOSTNAME = 'xmosaic'
VAGRANTFILE_API_VERSION = '2'
Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  config.ssh.forward_agent = true
  config.vm.box = 'ubuntu/bionic64'
  config.vm.hostname = HOSTNAME
  config.vm.provider 'virtualbox' do |vb|
    vb.name = HOSTNAME
  end
  config.vm.provision 'shell', inline: $script
end
