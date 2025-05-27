export {
	onSwitchSection,
	onCancelWifiConnect,
	onShowPassword,
	onWifiConnect,
	onWifiDisconnect,
	onScanForNetworks,
	onToggleLedDirection,
	getSensorData,
	getWifiStatus,
	onIntervalChange,
	onSetAdc
}

let baseUrl = '';

try {
	baseUrl = import.meta.env.VITE_API_URL;
} catch (e) {
	/* console.log('Erst mit Vite rendern'); */
}

let state = {
	api: {
		baseUrl,
		scan: 'scan',
		connect: 'connect',
		disconnect: 'disconnect',
		status: 'status',
		sensor: 'sensor',
		interval: 'interval',
		toggleWifi: 'toggleWifi',
		ledDirection: 'ledDirection',
		adc: 'adc'
	},
	wifi: {
		active: {
			ssid: null,
			signal: 0,
			channel: null,
			secured: false
		},
		ssid: '',
		password: '',
		secured: false
	},
	scannedNetworks: [],
	sensor: 0,
	activeSection: 'SectionSensorData'
};

let sensorDataRequested = false;

async function getSensorData() {
	if (state.activeSection === 'SectionSensorData' && !sensorDataRequested) {
		sensorDataRequested = true;

		try {
			const response = await fetch(state.api.baseUrl + state.api.sensor, {
				method: 'GET',
			})
			const data = await response.json();
			const value = parseInt(data.value, 10);
			const adcValue = parseInt(data.adcValue, 10);

			const inputSensorSignalValue = document.getElementById('InputSensorSignalValue');
			const sensorAdcMin = document.getElementById('SensorSignalAdcMin');
			const sensorAdcMax = document.getElementById('SensorSignalAdcMax');

			setIntervalSelection(data.interval);

			if (!inputSensorSignalValue.classList.contains('focused')) {
				inputSensorSignalValue.value = adcValue;
			}
			sensorAdcMin.innerHTML = `${data.adcMin}`;
			sensorAdcMax.innerHTML = `${data.adcMax}`;

			const sensorDigits = document.getElementById('SensorDigits');
			const digits = sensorDigits.querySelectorAll('.digit');

			sensorDigits.classList.toggle('alert', value === 0);
			sensorDigits.classList.toggle('low', value <= 1);
			sensorDigits.classList.toggle('medium', value > 1 && value <= 3);

			digits.forEach((element, index) => element.classList.toggle('active', value >= index));
		} catch (e) {
			return;
		} finally {
			sensorDataRequested = false;
		}
	}
}

function showLoader(value) {
	const loader = document.getElementById('Loader');
	if (value) {
		loader.classList.remove('d-none');
	} else {
		loader.classList.add('d-none');
	}
}

function onShowPassword() {
	const wifiPwdInput = document.getElementById('ModalWifiPassword');
	if (wifiPwdInput.type === 'password') {
		wifiPwdInput.type = 'text';
	} else {
		wifiPwdInput.type = 'password';
	}
}
function onSwitchSection(id) {
	console.log('[onSwitchSection]');
	const allSections = document.getElementsByClassName('section');
	const section = document.getElementById(id);
	Array.from(allSections).forEach((s) => s.classList.remove('show'));
	section.classList.add('show');
	if (id === 'SectionWifiSettings') {
		getWifiStatus();
	}
	state.activeSection = id;
}
async function onToggleLedDirection(event) {
	const toggle = document.getElementById('LedDirectionSwitch');
	if (toggle.checked) {
		await toggleLedDirection(true)
	} else {
		await toggleLedDirection(false)
	}
}

async function toggleLedDirection(status) {
	showLoader(true);
	try {
		await fetch(state.api.baseUrl + state.api.ledDirection, {
			method: 'POST',
			headers: { 'Content-Type': 'application/json' },
			body: JSON.stringify({ status })
		})
		return true;
	} catch (e) {
		return false;
	} finally {
		setTimeout(() => showLoader(false), 1000);
	}
}

async function onScanForNetworks() {
	const template = (ssid, channel, signal) => `<div class="d-flex ai-center ellipsed"><div class="icon"><svg xmlns="http://www.w3.org/2000/svg" width="26" height="26" fill="currentColor" viewBox="0 0 16 16"><path d="M13.229 8.271c.216-.216.194-.578-.063-.745A9.46 9.46 0 0 0 8 6c-1.905 0-3.68.56-5.166 1.526a.48.48 0 0 0-.063.745.525.525 0 0 0 .652.065A8.46 8.46 0 0 1 8 7a8.46 8.46 0 0 1 4.577 1.336c.205.132.48.108.652-.065m-2.183 2.183c.226-.226.185-.605-.1-.75A6.5 6.5 0 0 0 8 9c-1.06 0-2.062.254-2.946.704-.285.145-.326.524-.1.75l.015.015c.16.16.408.19.611.09A5.5 5.5 0 0 1 8 10c.868 0 1.69.201 2.42.56.203.1.45.07.611-.091zM9.06 12.44c.196-.196.198-.52-.04-.66A2 2 0 0 0 8 11.5a2 2 0 0 0-1.02.28c-.238.14-.236.464-.04.66l.706.706a.5.5 0 0 0 .708 0l.707-.707z"/></svg></div><div class="d-flex flex-column p-3"><div class="name mb-1">${ssid}</div><div class="channel small">Kanal: ${channel}</div></div></div><div class="d-flex ai-center"><div class="signal">${signal}%</div><div class="protected"><svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bibi-lock-fill" viewBox="0 0 16 16"><path d="M8 1a2 2 0 0 1 2 2v4H6V3a2 2 0 0 1 2-2m3 6V3a3 3 0 0 0-6 0v4a2 2 0 0 0-2 2v5a2 2 0 0 0 2 2h6a2 2 0 0 0 2-2V9a2 2 0 0 0-2-2"/></svg></div></div>`;

	showLoader(true);

	try {
		const response = await fetch(state.api.baseUrl + state.api.scan, {
			method: 'GET',
		})
		const networkList = await response.json();

		if (networkList) {
			const wifiNetworksResult = document.getElementById('WifiNetworksResult');
			const wifiList = document.getElementById("WifiList");
			state.scannedNetworks = networkList;

			wifiNetworksResult.classList.add('show');
			wifiList.innerHTML = "";
			state.scannedNetworks
				.sort((a, b) => a.signal > b.signal ? -1 : 1)
				.sort(n => state.wifi.active.ssid === n.ssid ? -1 : 1)
				.forEach(network => {
					const wifiElm = document.createElement("div");
					wifiElm.classList.add('wifi-item', 'd-flex', 'jc-between', 'ai-center');
					wifiElm.setAttribute('data-ssid', network.ssid);
					if (network.ssid === state.wifi.active.ssid) wifiElm.classList.add("active");
					if (network.secured) wifiElm.classList.add("secured");
					wifiElm.innerHTML = template(network.ssid, network.channel, network.signal);
					wifiElm.onclick = () => onSelectWifi(network);
					wifiList.appendChild(wifiElm);
				});
		}
	} catch (e) {
		alert('Uuups! Hier ist etwas schief gelaufen. Bitte nochmal probieren.');
	} finally {
		showLoader(false);
	}
}

function onSelectWifi({ ssid, secured }) {
	state.wifi.ssid = ssid;
	state.wifi.secured = secured;
	if (!secured) {
		onWifiConnect();
	} else {
		onShowWifiConnect();
	}
}

async function onWifiDisconnect() {
	console.log('[onWifiDisconnect]');
	showLoader(true);
	try {
		const response = await fetch(state.api.baseUrl + state.api.disconnect, {
			method: 'GET',
			timeout: 2000
		});
		const data = await response.json();
		setActiveWifi(data);
	} catch (e) {
		setActiveWifi({ connected: false });
		alert("Das Gerät wurde erfolgreich vom WLan getrennt. Du kannst diese Seite nun schließen und Dich mit dem Access-Point \"Sensor\" verbinden.")
	} finally {
		showLoader(false);
	}


}
async function onWifiConnect() {
	showLoader(true);
	const modal = document.getElementById('ModalWifi');
	const password = document.getElementById('ModalWifiPassword').value;
	state.wifi.password = password;
	try {
		const response = await fetch(state.api.baseUrl + state.api.connect, {
			method: 'POST',
			headers: { 'Content-Type': 'application/json' },
			body: JSON.stringify({ ssid: state.wifi.ssid, password: state.wifi.password }),
		});

		const data = await response.json();
		if (data?.connected) {
			modal.classList.remove('show');
			const wifiListItems = document.querySelectorAll('#WifiList .wifi-item');
			setActiveWifi(data);
			wifiListItems.forEach(node => {
				node.classList.remove('active');
				if (node.getAttribute('data-ssid') === state.wifi.active.ssid) {
					node.classList.add('active');
				}
			})
		} else {
			showError('Verbindung fehlgeschlagen. Bitte überprüfe deine Eingaben.');
		}
	} catch (error) {
		console.error("Error:", error);
		if (error.message === "Request timed out") {
			showError('Die Verbindung wurde unterbrochen. Bitte erneut versuchen.');
		} else {
			showError('Verbindungsfehler. Bitte prüfe das Passwort.');
		}
	} finally {
		showLoader(false);
	}
}

function showError(message) {
	const modalWifiError = document.getElementById('ModalWifiError');
	modalWifiError.innerHTML = message;
	modalWifiError.classList.add('show');
}
function onShowWifiConnect() {
	const modal = document.getElementById('ModalWifi');
	const modalWifiName = document.getElementById('ModalWifiName');
	const modalWifiPassword = document.getElementById('ModalWifiPassword');
	modalWifiPassword.addEventListener('input', onChangeWifiPassword);
	modalWifiName.innerHTML = state.wifi.ssid;
	modal.classList.add('show');
}
function onCancelWifiConnect() {
	const modal = document.getElementById('ModalWifi');
	const modalWifiPassword = document.getElementById('ModalWifiPassword');
	const modalWifiError = document.getElementById('ModalWifiError');
	modalWifiPassword.removeEventListener('input', onChangeWifiPassword);
	modalWifiError.classList.remove('show');
	modal.classList.remove('show');
	resetWifi();
}
function onChangeWifiPassword(e) {
	const modalWifiButtonConnect = document.getElementById('ModalWifiButtonConnect');
	modalWifiButtonConnect.disabled = !(e.target.value.length >= 8);
}

function resetWifi() {
	const modalWifiPassword = document.getElementById('ModalWifiPassword');
	modalWifiPassword.value = '';
	state.wifi = {
		active: {
			ssid: null,
			signal: 0,
			channel: null,
			secured: false
		}, ssid: '', password: '', secured: false
	};
}

async function getWifiStatus() {
	showLoader(true);
	try {
		const response = await fetch(state.api.baseUrl + state.api.status, {
			method: 'GET',
		})
		const data = await response.json();
		setActiveWifi(data);
		setMenuUpsideDown(data.menuUpsideDown)
	} catch (e) {
		alert('Uuups! Hier ist etwas schief gelaufen. Bitte nochmal probieren.');
	} finally {
		showLoader(false);
	}
}

function setMenuUpsideDown(value) {
	const menuToggle = document.getElementById('LedDirectionSwitch');
	menuToggle.checked = value;
}

function setActiveWifi(status) {
	const wifiConnected = document.getElementById('WifiConnected');
	const wifiName = wifiConnected.querySelector('.wifi-name');
	const wifiChannel = wifiConnected.querySelector('.wifi-channel');
	const wifiIP = wifiConnected.querySelector('.wifi-ip');
	const wifiSignal = wifiConnected.querySelector('.wifi-signal');
	if (status.connected && status.wifi) {
		state.wifi.active = status.wifi;
		wifiName.innerHTML = status.wifi.ssid;
		wifiIP.innerHTML = status.wifi.ip;
		wifiChannel.innerHTML = status.wifi.channel;
		wifiSignal.innerHTML = status.wifi.signal;
		wifiConnected.classList.remove('d-none');
	} else {
		wifiConnected.classList.add('d-none');
	}
}

function setIntervalSelection(value) {
	const intervalSelect = document.getElementById('IntervalSelect');

	if (!!intervalSelect && intervalSelect.getAttribute('data-updated') !== 'true') {
		intervalSelect.setAttribute('data-updated', 'true');
		intervalSelect.value = `${value}`;
	}
}

async function onIntervalChange(event) {
	showLoader(true);
	const interval = event.target.value;
	try {
		return await fetch(state.api.baseUrl + state.api.interval, {
			method: 'POST',
			headers: { 'Content-Type': 'application/json' },
			body: JSON.stringify({ interval }),
		});
	} catch (e) {
		return;
	} finally {
		setTimeout(() => showLoader(false), 1000);
	}
}

/**
 * 
 * @param {string} change "min" | "max"
 * @returns 
 */
async function onSetAdc(change) {
	showLoader(true);
	try {
		const inputSensorSignalValue = document.getElementById('InputSensorSignalValue');
		const value = inputSensorSignalValue.value;
		return await fetch(state.api.baseUrl + state.api.adc, {
			method: 'POST',
			headers: { 'Content-Type': 'application/json' },
			body: JSON.stringify({ change, value }),
		});
	} catch (e) {
		return;
	} finally {
		setTimeout(() => showLoader(false), 1000);
	}
}

