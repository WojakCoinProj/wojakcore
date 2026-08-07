#!/usr/bin/env bash
#
# DDoS Hardening for Seed Node Server
# For: miningpool.dedoo.xyz (DO droplet)
# Coins: WojakCoin (20759), JunkCoin (8333), LinkCoin (8334)
#
# Run as root: bash harden-seednode.sh
# IMPORTANT: Run this AFTER DO restores connectivity but BEFORE they unblock.
#            Or run via DO recovery console if available.

set -euo pipefail

echo "=== DDoS Hardening for Seed Node ==="
echo "WARNING: This will modify firewall rules and kernel settings."
echo "Press Ctrl+C to abort, or Enter to continue..."
read -r

# --- Configurable Ports ---
SSH_PORT="2244"                    # Change to non-standard SSH port
P2P_PORTS=(20759 8333 8334)       # WojakCoin, JunkCoin, LinkCoin
# RPC ports (if any) - keep localhost-only
RPC_PORTS=(8332 18332)

# --- 1. SYSTEM HARDENING: sysctl ---
echo "[1/6] Applying kernel hardening via sysctl..."

cat > /etc/sysctl.d/99-anti-ddos.conf << 'SYSCTL'
# IP Spoofing Protection
net.ipv4.conf.all.rp_filter = 1
net.ipv4.conf.default.rp_filter = 1

# Ignore ICMP redirects
net.ipv4.conf.all.accept_redirects = 0
net.ipv4.conf.default.accept_redirects = 0
net.ipv4.conf.all.secure_redirects = 0
net.ipv4.conf.default.secure_redirects = 0
net.ipv6.conf.all.accept_redirects = 0
net.ipv6.conf.default.accept_redirects = 0

# Ignore source-routed packets
net.ipv4.conf.all.accept_source_route = 0
net.ipv4.conf.default.accept_source_route = 0

# TCP SYN Cookie Protection (defends against SYN floods)
net.ipv4.tcp_syncookies = 1
net.ipv4.tcp_syn_retries = 3
net.ipv4.tcp_synack_retries = 2
net.ipv4.tcp_max_syn_backlog = 8192

# Reduce TIME_WAIT and increase port range
net.ipv4.tcp_fin_timeout = 15
net.ipv4.tcp_tw_reuse = 1
net.ipv4.ip_local_port_range = 1024 65535

# Connection tracking tuning
net.netfilter.nf_conntrack_max = 2000000
net.netfilter.nf_conntrack_tcp_timeout_established = 600
net.netfilter.nf_conntrack_tcp_timeout_syn_recv = 30
net.netfilter.nf_conntrack_tcp_timeout_syn_sent = 15
net.netfilter.nf_conntrack_tcp_timeout_fin_wait = 15
net.netfilter.nf_conntrack_tcp_timeout_time_wait = 15
net.netfilter.nf_conntrack_tcp_timeout_close = 5
net.netfilter.nf_conntrack_tcp_timeout_close_wait = 15
net.netfilter.nf_conntrack_tcp_timeout_last_ack = 15

# Increase socket buffers for high-throughput
net.core.rmem_max = 134217728
net.core.wmem_max = 134217728
net.core.rmem_default = 16777216
net.core.wmem_default = 16777216
net.core.optmem_max = 134217728
net.core.netdev_max_backlog = 100000
net.core.somaxconn = 65535

# IPv4 tuning
net.ipv4.tcp_rmem = 4096 87380 134217728
net.ipv4.tcp_wmem = 4096 65536 134217728
net.ipv4.tcp_congestion_control = cubic
net.ipv4.tcp_mtu_probing = 1
net.ipv4.tcp_sack = 1
net.ipv4.tcp_window_scaling = 1
net.ipv4.tcp_timestamps = 1
net.ipv4.tcp_fastopen = 3

# Ignore broadcast/multicast ICMP
net.ipv4.icmp_echo_ignore_broadcasts = 1
net.ipv4.icmp_ignore_bogus_error_responses = 1
net.ipv4.icmp_ratelimit = 500
net.ipv4.icmp_ratemask = 88089

# Increase max open files
fs.file-max = 1000000
fs.nr_open = 1000000
SYSCTL

sysctl -p /etc/sysctl.d/99-anti-ddos.conf

# Increase system limits
cat > /etc/security/limits.d/99-nofile.conf << 'LIMITS'
* soft nofile 1000000
* hard nofile 1000000
root soft nofile 1000000
root hard nofile 1000000
LIMITS

echo "   Kernel hardening applied."

# --- 2. FIREWALL: iptables ---
echo "[2/6] Configuring iptables firewall..."

# Flush existing rules
iptables -F
iptables -X
iptables -t nat -F
iptables -t nat -X
iptables -t mangle -F
iptables -t mangle -X

# Default policies: DROP incoming, ALLOW outgoing
iptables -P INPUT DROP
iptables -P FORWARD DROP
iptables -P OUTPUT ACCEPT

# Allow loopback
iptables -A INPUT -i lo -j ACCEPT

# Allow established/related connections
iptables -A INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT

# Allow SSH (current port 22 AND new port)
iptables -A INPUT -p tcp --dport 22 -m conntrack --ctstate NEW -m limit --limit 6/min --limit-burst 4 -j ACCEPT
iptables -A INPUT -p tcp --dport ${SSH_PORT} -m conntrack --ctstate NEW -m limit --limit 6/min --limit-burst 4 -j ACCEPT

# Allow P2P coin ports with rate limiting
for PORT in "${P2P_PORTS[@]}"; do
    echo "   Adding P2P port $PORT with rate limiting..."
    # Allow new connections but rate-limit SYN floods
    iptables -A INPUT -p tcp --dport ${PORT} -m conntrack --ctstate NEW -m limit --limit 300/sec --limit-burst 500 -j ACCEPT
    # Drop excessive SYN packets
    iptables -A INPUT -p tcp --dport ${PORT} --syn -m limit --limit 300/sec --limit-burst 500 -j ACCEPT
    iptables -A INPUT -p tcp --dport ${PORT} --syn -j DROP
done

# Rate-limit RPC ports (localhost only)
for PORT in "${RPC_PORTS[@]}"; do
    iptables -A INPUT -p tcp --dport ${PORT} -s 127.0.0.1 -j ACCEPT
    iptables -A INPUT -p tcp --dport ${PORT} -j DROP
done

# Drop common DDoS vectors
iptables -A INPUT -p tcp --tcp-flags ALL NONE -j DROP        # NULL packets
iptables -A INPUT -p tcp --tcp-flags ALL ALL -j DROP         # XMAS packets
iptables -A INPUT -p tcp --tcp-flags ALL SYN,RST,ACK,FIN,URG -j DROP
iptables -A INPUT -p tcp --tcp-flags SYN,FIN SYN,FIN -j DROP # SYN+FIN
iptables -A INPUT -p tcp --tcp-flags SYN,RST SYN,RST -j DROP # SYN+RST
iptables -A INPUT -p tcp --tcp-flags ALL FIN,URG,PSH -j DROP

# Drop fragments
iptables -A INPUT -f -j DROP

# Drop new non-SYN TCP packets
iptables -A INPUT -p tcp ! --syn -m conntrack --ctstate NEW -j DROP

# Allow ping (rate-limited)
iptables -A INPUT -p icmp --icmp-type echo-request -m limit --limit 1/sec --limit-burst 5 -j ACCEPT
iptables -A INPUT -p icmp --icmp-type echo-reply -j ACCEPT
iptables -A INPUT -p icmp --icmp-type destination-unreachable -j ACCEPT
iptables -A INPUT -p icmp --icmp-type time-exceeded -j ACCEPT

# Log and drop the rest (limit logging to avoid log flood)
iptables -A INPUT -m limit --limit 5/min -j LOG --log-prefix "iptables DROP: " --log-level 4

# Install iptables-persistent to save rules
if command -v apt-get &> /dev/null; then
    echo "   Saving iptables rules (Debian/Ubuntu)..."
    apt-get update -qq && apt-get install -y -qq iptables-persistent netfilter-persistent 2>/dev/null || true
    netfilter-persistent save 2>/dev/null || iptables-save > /etc/iptables/rules.v4
elif command -v yum &> /dev/null; then
    echo "   Saving iptables rules (RHEL/CentOS)..."
    iptables-save > /etc/sysconfig/iptables 2>/dev/null || true
fi

echo "   Firewall configured."

# --- 3. FAIL2BAN ---
echo "[3/6] Installing and configuring Fail2Ban..."

if command -v apt-get &> /dev/null; then
    apt-get install -y -qq fail2ban 2>/dev/null || true
elif command -v yum &> /dev/null; then
    yum install -y -q fail2ban 2>/dev/null || true
fi

cat > /etc/fail2ban/jail.local << FAIL2BAN
[DEFAULT]
bantime = 3600
findtime = 600
maxretry = 5
ignoreip = 127.0.0.1/8

[sshd]
enabled = true
port = 22,${SSH_PORT}
logpath = %(sshd_log)s
backend = %(sshd_backend)s
maxretry = 3
bantime = 86400

[sshd-ddos]
enabled = true
port = 22,${SSH_PORT}
logpath = %(sshd_log)s
backend = %(sshd_backend)s
maxretry = 6
findtime = 60

[recidive]
enabled = true
logpath = /var/log/fail2ban.log
banaction = %(banaction_allports)s
bantime = 604800
findtime = 86400
maxretry = 3
FAIL2BAN

# Restart fail2ban
if systemctl is-active --quiet fail2ban; then
    systemctl restart fail2ban
else
    systemctl enable fail2ban
    systemctl start fail2ban
fi

echo "   Fail2Ban configured."

# --- 4. SSH HARDENING ---
echo "[4/6] Hardening SSH configuration..."

cp /etc/ssh/sshd_config /etc/ssh/sshd_config.bak.$(date +%s)

# Change SSH port and harden
sed -i "s/^#Port 22/Port ${SSH_PORT}/" /etc/ssh/sshd_config
sed -i 's/^#PermitRootLogin.*/PermitRootLogin prohibit-password/' /etc/ssh/sshd_config
sed -i 's/^#PasswordAuthentication.*/PasswordAuthentication no/' /etc/ssh/sshd_config
sed -i 's/^#ChallengeResponseAuthentication.*/ChallengeResponseAuthentication no/' /etc/ssh/sshd_config
sed -i 's/^#UsePAM.*/UsePAM yes/' /etc/ssh/sshd_config
sed -i 's/^#X11Forwarding.*/X11Forwarding no/' /etc/ssh/sshd_config
sed -i 's/^#MaxAuthTries.*/MaxAuthTries 3/' /etc/ssh/sshd_config
sed -i 's/^#MaxSessions.*/MaxSessions 5/' /etc/ssh/sshd_config
sed -i 's/^#MaxStartups.*/MaxStartups 3:50:10/' /etc/ssh/sshd_config
sed -i 's/^#ClientAliveInterval.*/ClientAliveInterval 300/' /etc/ssh/sshd_config
sed -i 's/^#ClientAliveCountMax.*/ClientAliveCountMax 2/' /etc/ssh/sshd_config
sed -i 's/^#LoginGraceTime.*/LoginGraceTime 30/' /etc/ssh/sshd_config

# Also ensure port 22 still works during transition
if ! grep -q "^Port 22" /etc/ssh/sshd_config && ! grep -q "^Port ${SSH_PORT}" /etc/ssh/sshd_config; then
    # Add both ports for transition
    echo "" >> /etc/ssh/sshd_config
    echo "Port 22" >> /etc/ssh/sshd_config
    echo "Port ${SSH_PORT}" >> /etc/ssh/sshd_config
fi

systemctl restart sshd 2>/dev/null || service sshd restart 2>/dev/null || service ssh restart 2>/dev/null

echo "   SSH hardened. NEW SSH PORT: ${SSH_PORT}"
echo "   IMPORTANT: Test new SSH port before closing old one!"

# --- 5. LIMIT SYSTEMD RESOURCES FOR DAEMONS ---
echo "[5/6] Applying systemd resource limits..."

for coin in wojakcoin junkcoin linkcoin; do
    SERVICE_FILE="/etc/systemd/system/${coin}d.service"
    if [ -f "$SERVICE_FILE" ]; then
        mkdir -p /etc/systemd/system/${coin}d.service.d
        cat > "/etc/systemd/system/${coin}d.service.d/limits.conf" << LIMITS
[Service]
LimitNOFILE=1000000
LimitNPROC=65535
LimitMEMLOCK=infinity
LIMITS
        echo "   Resource limits applied to ${coin}d"
    fi
done

systemctl daemon-reload

# --- 6. SUMMARY ---
echo ""
echo "=============================================="
echo "  HARDENING COMPLETE"
echo "=============================================="
echo ""
echo "  NEW SSH PORT: ${SSH_PORT}"
echo "  P2P Ports:    ${P2P_PORTS[*]}"
echo ""
echo "  NEXT STEPS:"
echo "  1. Test SSH on new port: ssh -p ${SSH_PORT} root@miningpool.dedoo.xyz"
echo "  2. Set up DO Cloud Firewall (do this NOW in DO panel):"
echo "     - ALLOW TCP ${SSH_PORT} from YOUR IP ONLY"
echo "     - ALLOW TCP ${P2P_PORTS[*]} from 0.0.0.0/0"
echo "     - ALLOW ICMP from 0.0.0.0/0"
echo "     - DROP all other inbound"
echo "  3. Add DO Floating IP as a DDoS mitigation layer"
echo "  4. Consider Cloudflare Magic Transit for production"
echo ""
echo "  Firewall rules saved. Reboot to verify persistence."
echo "=============================================="
