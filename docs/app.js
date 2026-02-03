import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { RoomEnvironment } from 'three/addons/environments/RoomEnvironment.js';

// --- CONFIGURATION ---
const SERVICE_UUID = "19B10000-0000-0000-0000-000000000000";
const UUIDS = {
    heading: "19b10001-0000-0000-0000-000000000000" // Sends [Heading, Pitch, Roll] as 3 floats
};

// --- THREE.JS GLOBALS ---
let scene, camera, renderer, controls;
let targetObject; // The object we rotate
let defaultBoard; // The fallback object

// --- STATE ---
let isConnected = false;
let currentRotation = { h: 0, p: 0, r: 0 };

init();
animate();

function log(msg) {
    const consoleBox = document.getElementById('console-log');
    if (!consoleBox) return;
    const time = new Date().toLocaleTimeString();
    const line = document.createElement('div');
    line.innerText = `[${time}] ${msg}`;
    consoleBox.prepend(line);
    console.log(`[${time}] ${msg}`);
}

function init() {
    // 1. Scene Setup
    const container = document.getElementById('canvas-container');
    scene = new THREE.Scene();

    // 2. Camera
    camera = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 0.1, 1000);
    camera.position.set(0, 5, 10);
    camera.lookAt(0, 0, 0);

    // 3. Renderer
    renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
    renderer.setSize(window.innerWidth, window.innerHeight);
    renderer.setPixelRatio(window.devicePixelRatio);
    renderer.setClearColor(0x000000, 0); // Transparent background to show CSS gradient
    renderer.toneMapping = THREE.ACESFilmicToneMapping;
    renderer.toneMappingExposure = 0.7;
    container.appendChild(renderer.domElement);

    // 4. Environment & Lights (Improved for PBR)
    const pmremGenerator = new THREE.PMREMGenerator(renderer);
    scene.environment = pmremGenerator.fromScene(new RoomEnvironment(renderer), 0.04).texture;

    const ambientLight = new THREE.AmbientLight(0xffffff, 0.5);
    scene.add(ambientLight);

    const dirLight = new THREE.DirectionalLight(0xffffff, 1.5);
    dirLight.position.set(5, 10, 7);
    scene.add(dirLight);

    const backLight = new THREE.DirectionalLight(0xffffff, 1);
    backLight.position.set(-5, 5, -5);
    scene.add(backLight);

    // 5. Grid Helper
    const gridHelper = new THREE.GridHelper(20, 20, 0x333333, 0x111111);
    scene.add(gridHelper);

    // 6. Controls
    controls = new OrbitControls(camera, renderer.domElement);
    controls.enableDamping = true;
    controls.dampingFactor = 0.05;

    // 7. Load Default Object
    loadGLTF('glb-3347.glb');

    // 8. Event Listeners
    window.addEventListener('resize', onWindowResize);
    document.getElementById('connectBtn').addEventListener('click', connectBluetooth);
    document.getElementById('disconnectBtn').addEventListener('click', disconnectBluetooth);

    // Model Selector Logic
    const modelSelect = document.getElementById('modelSelect');
    const dropZone = document.getElementById('dropZone');

    modelSelect.addEventListener('change', (e) => {
        const value = e.target.value;
        if (value === 'custom') {
            dropZone.classList.remove('hidden');
        } else {
            dropZone.classList.add('hidden');
            loadGLTF(value);
        }
    });

    // Exposure Slider Logic
    const exposureSlider = document.getElementById('exposureSlider');
    exposureSlider.addEventListener('input', (e) => {
        const val = parseFloat(e.target.value);
        renderer.toneMappingExposure = val;
    });

    // File Drop & Input
    const fileInput = document.getElementById('fileInput');

    dropZone.addEventListener('click', () => fileInput.click());
    dropZone.addEventListener('dragover', (e) => { e.preventDefault(); dropZone.style.borderColor = '#3b82f6'; });
    dropZone.addEventListener('dragleave', () => { dropZone.style.borderColor = 'rgba(255,255,255,0.1)'; });
    dropZone.addEventListener('drop', handleDrop);
    fileInput.addEventListener('change', handleFileSelect);

    log("System initialized. Ready to connect.");
}

// Removed createDefaultBoard as we use external files now

function onWindowResize() {
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);
}

function animate() {
    requestAnimationFrame(animate);

    // Smooth interpolation could be added here
    if (targetObject) {

        const degToRad = Math.PI / 180;

        // We set rotation.
        targetObject.rotation.set(
            currentRotation.p * degToRad,  // X (Pitch)
            -currentRotation.h * degToRad, // Y (Heading)
            currentRotation.r * degToRad   // Z (Roll)
        );
    }

    controls.update();
    renderer.render(scene, camera);
}

// --- FILE HANDLING ---
function handleDrop(e) {
    e.preventDefault();
    document.getElementById('dropZone').style.borderColor = 'rgba(255,255,255,0.1)';
    const file = e.dataTransfer.files[0];
    loadModel(file);
}

function handleFileSelect(e) {
    const file = e.target.files[0];
    loadModel(file);
}

function loadModel(file) {
    if (!file) return;

    log(`Loading local file: ${file.name}`);
    const extension = file.name.split('.').pop().toLowerCase();

    if (extension === 'glb' || extension === 'gltf') {
        const url = URL.createObjectURL(file);
        loadGLTF(url);
    } else {
        alert("Only .glb and .gltf formats are supported.");
    }
}

function loadGLTF(url) {
    log(`Loading model...`);
    const loader = new GLTFLoader();
    loader.load(url, (gltf) => {
        if (targetObject) scene.remove(targetObject);

        targetObject = gltf.scene;

        // Center the model
        const box = new THREE.Box3().setFromObject(targetObject);
        const center = box.getCenter(new THREE.Vector3());
        targetObject.position.sub(center); // Center at 0,0,0

        scene.add(targetObject);
        log("Model loaded successfully.");
    }, undefined, (error) => {
        console.error(error);
        log("Error loading model: " + error.message);
        // Fallback or alert?
    });
}


// --- BLUETOOTH LOGIC ---
let device, server, niclaService;

async function connectBluetooth() {
    const statusPill = document.getElementById('connectionStatus');
    const connectBtn = document.getElementById('connectBtn');

    if (!navigator.bluetooth) {
        log("Web Bluetooth API not available in this browser.");
        alert("Web Bluetooth API not available.");
        return;
    }

    try {
        log("Requesting Bluetooth Device...");

        device = await navigator.bluetooth.requestDevice({
            filters: [{ namePrefix: 'Nicla' }],
            optionalServices: [SERVICE_UUID.toLowerCase()]
        });

        log(`Device selected: ${device.name}`);
        device.addEventListener('gattserverdisconnected', onDisconnected);

        log("Connecting to GATT Server (10s timeout)...");

        // Race condition: Connect OR Timeout
        const connectPromise = device.gatt.connect();

        const timeoutPromise = new Promise((_, reject) =>
            setTimeout(() => reject(new Error("Connection timed out. Is the device connected elsewhere?")), 10000)
        );

        server = await Promise.race([connectPromise, timeoutPromise]);

        log("GATT connected successfully.");

        log("Getting Primary Service...");
        niclaService = await server.getPrimaryService(SERVICE_UUID.toLowerCase());
        log("Service found.");

        log("Getting Heading Characteristic...");
        const characteristic = await niclaService.getCharacteristic(UUIDS.heading);
        log("Characteristic found. Starting notifications...");

        await characteristic.startNotifications();
        characteristic.addEventListener('characteristicvaluechanged', handleOrientationChange);
        log("Notifications started. Listening for data...");

        // UI Updates
        isConnected = true;
        statusPill.classList.add('connected');
        statusPill.querySelector('.text').innerText = "Connected";

        connectBtn.classList.add('hidden');
        document.getElementById('disconnectBtn').classList.remove('hidden');
        document.getElementById('dataCard').classList.remove('hidden');

    } catch (error) {
        console.error(error);
        log(`Connection Error: ${error.message}`);
        alert("Connection Failed: " + error.message + "\n\nTry pressing the RESET button on the Nicla board.");

        // Force disconnect if partial state
        if (device && device.gatt.connected) device.gatt.disconnect();
    }
}

function disconnectBluetooth() {
    if (device && device.gatt.connected) {
        log("Disconnecting...");
        device.gatt.disconnect();
    }
}

function onDisconnected() {
    log("Device Disconnected.");
    isConnected = false;
    const statusPill = document.getElementById('connectionStatus');
    statusPill.classList.remove('connected');
    statusPill.querySelector('.text').innerText = "Disconnected";

    document.getElementById('connectBtn').classList.remove('hidden');
    document.getElementById('disconnectBtn').classList.add('hidden');
    document.getElementById('dataCard').classList.add('hidden');
}

function handleOrientationChange(event) {
    const value = event.target.value;

    // The firmware sends 3 floats (Little Endian): [Heading, Pitch, Roll]
    // Log the raw data length only once appropriately or throttle it
    // log(`Data received: ${value.byteLength} bytes`);

    const h = value.getFloat32(0, true);
    const p = value.getFloat32(4, true);
    const r = value.getFloat32(8, true);

    // Update State
    currentRotation = { h, p, r };

    // Update UI
    document.getElementById('valHeading').innerText = h.toFixed(1) + "°";
    document.getElementById('valPitch').innerText = p.toFixed(1) + "°";
    document.getElementById('valRoll').innerText = r.toFixed(1) + "°";
}
