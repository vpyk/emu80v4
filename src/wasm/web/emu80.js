const iframe = document.getElementById("iframe");
const platformSelect = document.getElementById("platformSelect");
const fileSelect = document.getElementById("fileSelect");
const fileSelectDiv = document.getElementById("fileSelectDiv");
const infoDiv = document.getElementById("infoDiv");
const caption = document.getElementById("caption");

const wideButton = document.getElementById("wideButton");
const parButton = document.getElementById("parButton");
const colorSelect = document.getElementById("colorSelect");
const fileButton = document.getElementById("fileButton");
const diskAButton = document.getElementById("diskAButton");
const resetButton = document.getElementById("resetButton");
const pauseButton = document.getElementById("pauseButton");
const scalingSelect = document.getElementById("scalingSelect");
const visibleAreaButton = document.getElementById("visibleAreaButton");
const smoothingSelect = document.getElementById("smoothingSelect");
const layoutSelect = document.getElementById("layoutSelect");
const fontButton = document.getElementById("fontButton");
const muteButton = document.getElementById("muteButton");
const fullscreenButton = document.getElementById("fullscreenButton");

const screenModeSpan = document.getElementById("screenModeSpan");
const dmaSpan = document.getElementById("dmaSpan");

const extraIframeSize = (parseInt(getComputedStyle(iframe).getPropertyValue("border-width")) + 
                        parseInt(getComputedStyle(iframe).getPropertyValue("padding"))) * 2;

let curPlatform = "";

let infoTimer = null;

const fileInput = document.createElement("input");
fileInput.type='file'
fileInput.style.display='none'

let platforms = []
let files = []


iframe.addEventListener("load", () => {
        const innerDoc = iframe.contentDocument || iframe.contentWindow.document;
        const canvas = innerDoc.getElementById("canvas");

        iframe.contentWindow.addEventListener("focus", () => {
            iframe.style.borderColor = "#888";
        })

        iframe.contentWindow.addEventListener("blur", () => {
            iframe.style.borderColor = "#eee";
        })

        curPlatform = platformSelect.value;

        adjustControls();

        iframe.focus();

        if (!iframe.contentWindow.location.pathname.includes("dummyframe")) {
            document.addEventListener("fullscreenchange", () => {
                if (!document.fullscreenElement) {
                    iframe.contentWindow.Module._wasmEmuExitFullscreenMode();
                }
            });
            
            iframe.contentDocument.addEventListener("keydown", (event) => {
                const key = event.key;
                switch (key) {
                    /*case "ArrowUp":
                    case "ArrowDown":
                    case "ArrowLeft":
                    case "ArrowRight":*/
                    case "End":
                    case "Home":
                    case "PageUp":
                    case "PageDown":
                event.preventDefault();
                }
                // Alt-Home,Up,Down
                if (event.altkey && (event.keyCode == 0x24 || event.keyCode == 0x26 || event.keyCode == 0x28))
                    event.preventDefault();
            });
        }
    })


window.addEventListener("popstate", () => {processParams(false)});


setInterval( () => {
        const canvasHeight = iframe.contentWindow.document.getElementById("canvas").height;
        const canvasWidth = iframe.contentWindow.document.getElementById("canvas").width;

        if (iframe.height != canvasHeight)
            iframe.style.height = canvasHeight + extraIframeSize + "px";

        if (iframe.width != canvasWidth)
            iframe.style.width = canvasWidth + extraIframeSize + "px";

        if (iframe.contentDocument.title && document.title != iframe.contentDocument.title) {
            document.title = iframe.contentDocument.title;
            caption.innerHTML = iframe.contentDocument.title;
        }
    }, 100);


fetch("catalog/platforms.json")
    .then(response => response.json())
    .then(data => fillPlatformsFromJson(data))
    .catch(error => console.error("Error reading platform list: ", error))
    .finally(() => processParams());


function fillPlatformsFromJson(data) {
    platforms = data;

    for (const platform of data) {
        const option = document.createElement("option");
        option.value = platform.name;
        option.innerHTML = platform.description;
        platformSelect.append(option);
    }
}


function processParams(saveState = true)
{
    const params = new URLSearchParams(window.location.search);

    let platform = params.get("platform");
    let file = params.get("run");

    if (platform === null) platform = "";
    if (file === null) file = "";

    platformSelect.value = platform;
    platformChange().then( () => {
        fileSelect.value = file.split("/").at(-1);
    });

    const programParam = file ? `run=${file}` : "";

    if (platform)
        run([`platform=${platform}`, programParam], saveState)
    else
        run([], saveState)
}


function platformChange() {
    for(let i = fileSelect.options.length - 1; i >= 1; i--)
        fileSelect.remove(i);

    if (platformSelect.value) {
        const platform = platforms.find(item => item.name === platformSelect.value);
        if (platform) {
            fileInput.setAttribute("accept", platform.extensions);
        } else {
            fileInput.setAttribute("accept", ".*");
        }

        fileSelectDiv.style.visibility = "visible"
        return fillFiles(platformSelect.value)
    } else
        fileSelectDiv.style.visibility = "hidden"
        return Promise.resolve()
}


function fillFiles(platform)
{
    return fetch(`catalog/${platform}/files.json`)
           .then(response => response.json())
           .then(data => fillFilesFromJson(data))
           .catch(error => console.error("Error reading file list: ", error))
}


function fillFilesFromJson(data) {
    files = data;

    for (const file of data) {
        const option = document.createElement("option");
        option.value = file.name;
        option.innerHTML = `${file.description} (${file.name})`;
        fileSelect.append(option);
    }
}


function fileChange() {
    //
}


function runPlatform() {
    const platform = platformSelect.value;

    stopInfoTimer();

    if (platform)
        run([`platform=${platform}`]);
    else
        run([]);
}


function run(args, saveState = true) {
    let query = "";
    if (args.length) {
        for (arg of args) {
            if (!arg)
                continue;
            if (!query)
                query += "?" + arg;
            else
                query += "&" + arg;
        }
        iframe.contentWindow.location.replace(`emuframe.html${query}`);
    } else {
        iframe.contentWindow.location.replace("dummyframe.html");
    }

    if (saveState) {
        history.pushState({}, "", `${document.location.pathname}${query}`)
    }
}


function runFile() {
    const platform = platformSelect.value;
    const file = fileSelect.value;

    const fileParams = files.find(item => item.name === file);
    const postConfParam = fileParams && fileParams.postconf ? `post-conf=catalog/options/${fileParams.postconf}` : "";

    if (platform && file) {
        stopInfoTimer();
        run([`platform=${platform}`, `run=catalog/${platform}/${file}`, postConfParam])
    }
}


function readFile(event) {
    if (!curPlatform)
        return;

    file = event.target.files[0];

    const reader = new FileReader();

    reader.onload = (event) => {
        const uint8Arr = new Uint8Array(event.target.result);
        const numBytes = uint8Arr.length * uint8Arr.BYTES_PER_ELEMENT;
        const fileId = iframe.contentWindow.Module._wasmEmuAllocateFileBuf(numBytes);
        const dataPtr = iframe.contentWindow.Module._wasmEmuGetFileBufPtr(fileId);
        const heapData = new Uint8Array(iframe.contentWindow.Module.HEAPU8.buffer, dataPtr, numBytes);
        heapData.set(uint8Arr);

        iframe.contentWindow.Module._wasmEmuOpenFile();

        fileInput.value = null;
      };

    reader.readAsArrayBuffer(file);
}


async function openFile(event) {
    if (!curPlatform)
        return;

    const fileId = await selectAndLoadFile();

    if (fileId) {
        iframe.contentWindow.Module._wasmEmuOpenFile(fileId);
        iframe.focus();
    }
}


function emuSysReq(id) {
    if (!curPlatform)
        return;

    iframe.contentWindow._wasmEmuSysReq(id);
    iframe.focus();
}


function emuSysReqNotFocus(id) {
    if (!curPlatform)
        return;

    iframe.contentWindow._wasmEmuSysReq(id);
}


async function selectAndLoadFile() {
    window.addEventListener("focus", windowFocusChange);
    iframe.contentWindow.addEventListener("focus", windowFocusChange);

    fileInput.value = null;
    fileInput.click();

    await new Promise((resolve) => {
        fileInput.onchange = (e) => resolve();
    });

    window.removeEventListener("focus", windowFocusChange);
    iframe.contentWindow.removeEventListener("focus", windowFocusChange);

    if (!fileInput.value) {
        console.log("File not selected!");
        return 0;
    }

    const file = fileInput.files[0];

    const result = await new Promise((resolve) => {
        const fileReader = new FileReader();
        fileReader.onloadend = (e) => resolve(fileReader.result);
        fileReader.readAsArrayBuffer(file);
    });

    if (!result) {
        console.log("Error reading file!");
        return 0;
    }

    fileInput.value = null;

    const uint8Arr = new Uint8Array(result);
    const numBytes = uint8Arr.length * uint8Arr.BYTES_PER_ELEMENT;
    const fileId = iframe.contentWindow.Module._wasmEmuAllocateFileBuf(numBytes);
    const dataPtr = iframe.contentWindow.Module._wasmEmuGetFileBufPtr(fileId);
    const heapData = new Uint8Array(iframe.contentWindow.Module.HEAPU8.buffer, dataPtr, numBytes);
    heapData.set(uint8Arr);

    return fileId;
}


function windowFocusChange() {
    window.removeEventListener("focus", windowFocusChange);
    iframe.contentWindow.removeEventListener("focus", windowFocusChange);
    setTimeout( () => {
        fileInput.onchange();
        iframe.focus();
    }, 200);
}


function getPlatformPropertyValue(object, property) {
    const obj = curPlatform + "." + object;
    const value = iframe.contentWindow.Module.ccall("wasmEmuGetPropertyValue",
                                                    "string", ["string", "string"],
                                                     [obj, property]);
   return value; 
}


function setPlatformPropertyValue(object, property, value) {
    const obj = curPlatform + "." + object;
    iframe.contentWindow.Module.ccall("wasmEmuSetPropertyValue",
                                                    "void", ["string", "string", "string"],
                                                     [obj, property, value]);
   return value; 
}


function updateConfig()
{
    adjustButtonStates();
    startInfoTimer();
}


function quitRequest()
{
    platformSelect.value = "";
    platformChange();
    run([]);
}


function toggleButton(button)
{
    if (button.classList.contains("is-active")) {
        button.classList.remove("is-active");
        return false;
    } else {
        button.classList.add("is-active");
        return true;
    }
}


function adjustButtonState(buttonId, obj, property, trueValue)
{
    const state = getPlatformPropertyValue(obj, property) == trueValue;
    const button = document.getElementById(buttonId);
    if (state)
        button.classList.add("is-active");
    else
        button.classList.remove("is-active");
}


function adjustButtonStates()
{
    adjustButtonState("visibleAreaButton", "crtRenderer", "visibleArea", "yes");
    adjustButtonState("parButton", "window", "aspectCorrection", "yes");
    adjustButtonState("wideButton", "window", "wideScreen", "yes");
    adjustButtonState("fontButton", "crtRenderer", "altRenderer", "yes");

    if (parButton.classList.contains("is-active"))
        wideButton.removeAttribute("disabled");
    else
        wideButton.setAttribute("disabled", "disabled");

    layoutSelect.value = getPlatformPropertyValue("kbdLayout", "layout");
    smoothingSelect.value = getPlatformPropertyValue("window", "smoothing");
    scalingSelect.value = getPlatformPropertyValue("window", "frameScale");
    colorSelect.value = getPlatformPropertyValue("crtRenderer", "colorMode");

    // Пока здесь,но достаточно один раз при запуске платформы
    const label = getPlatformPropertyValue("diskA", "label");
    if (label)
        diskAButton.removeAttribute("disabled");
    else
        diskAButton.setAttribute("disabled", "disabled");

    const altFont = getPlatformPropertyValue("crtRenderer", "altRenderer");
    if (altFont)
        fontButton.removeAttribute("disabled");
    else
        fontButton.setAttribute("disabled", "disabled");

}


function onMute(button)
{
    toggleButton(button);
    emuSysReq(SR_MUTE);
}


function onPause(button)
{
    res = toggleButton(button);
    emuSysReq(res ? SR_PAUSEON : SR_PAUSEOFF);
}


function onInfo(button)
{
    res = toggleButton(button);
    infoDiv.style.display = res ? "" : "none";
}


function onLayoutChange(select)
{
    setPlatformPropertyValue("kbdLayout", "layout", select.value);
    iframe.focus();
}


function onSmoothingChange(select)
{
    setPlatformPropertyValue("window", "smoothing", select.value);
    iframe.focus();
}


function onScalingChange(select)
{
    setPlatformPropertyValue("window", "frameScale", select.value);
    iframe.focus();
}


function onColorChange(select)
{
    setPlatformPropertyValue("crtRenderer", "colorMode", select.value);
    iframe.focus();
}


const commonColorModes = [
    ["mono", "Ч/б"],
    ["color", "Цветной"]
];

const rkColorModes = [
    ["original", "Ч/б ориг"],
    ["mono", "Ч/б"],
    ["color1", "Цв. (Толкалин)"],
    ["color2", "Цв. (Акименко)"]
];

const kr04ColorModes = [
    ["mono", "Ч/б"],
    ["color", "Цветной"],
    ["colorModule", "Блок цвета"]
];

const platformColorModes = {
    rk86: rkColorModes,
    kr04: kr04ColorModes,
    apogey: commonColorModes,
    pk8000: commonColorModes,
    vector: commonColorModes
}


const emuControls = [
    wideButton,
    parButton,
    colorSelect,
    fileButton,
    diskAButton,
    resetButton,
    pauseButton,
    scalingSelect,
    visibleAreaButton,
    smoothingSelect,
    layoutSelect,
    fontButton,
    muteButton,
    fullscreenButton,
    infoButton
]; 


function adjustControls()
{
    for (const control of emuControls) {
        if (curPlatform)
            control.removeAttribute("disabled");
        else
            control.setAttribute("disabled", "disabled");
    }

    infoDiv.style.display = (curPlatform && infoButton.classList.contains("is-active")) ? "" : "none";

    for(let i = colorSelect.options.length - 1; i >= 0; i--)
        colorSelect.remove(i);

    const colorModes = platformColorModes[curPlatform];

    if (!colorModes || !curPlatform) {
        colorSelect.setAttribute("disabled", "disabled");
    } else {
        colorSelect.removeAttribute("disabled");
        for (const color of colorModes) {
            const option = document.createElement("option");
            option.value = color[0];
            option.innerHTML = color[1];
            colorSelect.append(option);
        }
    colorSelect.value = null;
    }
}


function startInfoTimer()
{
    infoTimer = setInterval(updateInfo, 500);
}


function stopInfoTimer()
{
    clearInterval(infoTimer);
    infoTimer = null;
}


function updateInfo()
{
    if (infoDiv.style.display)
        return;

    const crtMode= getPlatformPropertyValue("crtRenderer", "crtMode");
    const dmaTime = getPlatformPropertyValue("dma", "percentage");
    
    screenModeSpan.innerHTML = crtMode;
    dmaSpan.innerHTML = dmaTime;
}
