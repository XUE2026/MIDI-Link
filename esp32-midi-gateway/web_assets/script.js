let authToken = null;
let statusTimer = null;

document.querySelectorAll('.tab-btn').forEach(btn => {
    btn.addEventListener('click', () => {
        document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
        document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
        btn.classList.add('active');
        document.getElementById('tab-' + btn.dataset.tab).classList.add('active');
    });
});

function api(path, data) {
    const opts = {
        method: data ? 'POST' : 'GET',
        headers: { 'Content-Type': 'application/json' },
    };
    if (data) opts.body = JSON.stringify(data);
    return fetch(path, opts).then(r => r.json());
}

function doLogin() {
    const user = document.getElementById('login-user').value;
    const pass = document.getElementById('login-pass').value;
    api('/api/login', { username: user, password: pass }).then(res => {
        if (res.success) {
            document.getElementById('login-page').classList.add('hidden');
            document.getElementById('dashboard-page').classList.remove('hidden');
            document.getElementById('status-indicator').className = 'status-dot online';
            refreshStatus();
            loadConfig();
            statusTimer = setInterval(refreshStatus, 5000);
        } else {
            document.getElementById('login-error').classList.remove('hidden');
        }
    });
}

function refreshStatus() {
    api('/api/status').then(res => {
        if (res.ip) {
            document.getElementById('s-ip').textContent = res.ip;
            document.getElementById('s-mode').textContent = res.mode || '--';
            document.getElementById('s-midi-source').textContent = res.midi_source || '--';
            document.getElementById('s-usb').textContent = res.usb_connected ? '已连接' : '未连接';
            document.getElementById('s-ble').textContent = res.ble_connected ? '已连接' : '未连接';
        }
    });
}

function loadConfig() {
    api('/api/config').then(res => {
        if (res.wifi_ssid) {
            document.getElementById('cfg-wifi-ssid').value = res.wifi_ssid;
            document.getElementById('cfg-wifi-mode').value = res.softap_mode ? '1' : '0';
            document.getElementById('cfg-midi-input').value = res.midi_input;
            document.getElementById('cfg-midi-output').value = res.midi_output;
            document.getElementById('cfg-udp-ip').value = res.udp_target_ip;
            document.getElementById('cfg-udp-port').value = res.udp_port;
            document.getElementById('cfg-encryption').value = res.encryption;
            document.getElementById('cfg-auth-mode').value = res.auth_mode;
            document.getElementById('cfg-ip-whitelist').value = res.ip_whitelist;
        }
    });
}

function saveConfig() {
    const data = {
        wifi_ssid: document.getElementById('cfg-wifi-ssid').value,
        wifi_password: document.getElementById('cfg-wifi-password').value || '******',
        softap_mode: document.getElementById('cfg-wifi-mode').value === '1',
        midi_input: parseInt(document.getElementById('cfg-midi-input').value),
        midi_output: parseInt(document.getElementById('cfg-midi-output').value),
        udp_target_ip: document.getElementById('cfg-udp-ip').value,
        udp_port: parseInt(document.getElementById('cfg-udp-port').value),
        encryption: parseInt(document.getElementById('cfg-encryption').value),
        auth_mode: parseInt(document.getElementById('cfg-auth-mode').value),
        ip_whitelist: document.getElementById('cfg-ip-whitelist').value,
    };
    api('/api/config', data).then(res => {
        alert(res.success ? '配置已保存' : '保存失败');
        refreshStatus();
    });
}

function scanBLE() {
    api('/api/ble-scan', {}).then(res => {
        alert(res.success ? 'BLE扫描已触发' : '扫描失败');
    });
}

function changeSSHPassword() {
    const data = {
        old_password: document.getElementById('ssh-old-pwd').value,
        new_password: document.getElementById('ssh-new-pwd').value,
        sensitive_password: document.getElementById('ssh-sensitive-pwd').value,
    };
    if (!data.new_password) { alert('请输入新密码'); return; }
    api('/api/change-ssh-password', data).then(res => {
        alert(res.success ? 'SSH密码已修改' : '修改失败，请检查密码');
    });
}

function emergencyBrake(action) {
    const data = { action: action };
    if (action === 'release') {
        data.password = document.getElementById('brake-password').value;
    }
    api('/api/emergency-brake', data).then(res => {
        alert(res.success ? (action === 'engage' ? '已紧急制动' : '已解除制动') : '操作失败');
    });
}

function resetDevice() {
    const data = {
        confirm_text: document.getElementById('reset-confirm-text').value,
        reset_password: document.getElementById('reset-pwd').value,
        sensitive_password: document.getElementById('reset-sensitive-pwd').value,
    };
    api('/api/reset-device', data).then(res => {
        if (res.success) {
            alert('设备正在重置...');
            location.reload();
        } else {
            alert('重置失败，请检查确认步骤');
        }
    });
}

function doOTA() {
    const fileInput = document.getElementById('ota-file');
    const backup = document.getElementById('ota-backup').checked;
    if (!fileInput.files.length) { alert('请选择固件文件'); return; }
    const formData = new FormData();
    formData.append('firmware', fileInput.files[0]);
    fetch('/api/ota-upload?backup=' + (backup ? '1' : '0'), { method: 'POST', body: formData })
        .then(r => r.json()).then(res => {
            alert(res.success ? '升级中，设备将重启...' : '升级失败');
        });
}