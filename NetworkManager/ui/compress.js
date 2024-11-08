const { gzip } = require('@gfx/zopfli');
const FS = require('fs');
const path = require('path');

const HTML = `
<head>
    <title>Easy Network Manager Wifi Portal</title>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
    <meta http-equiv=X-UA-Compatible content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <link rel="icon" href="favicon.ico" />
    <style>
        html {
            font-family: Arial, Helvetica, sans-serif, -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, 'Open Sans', 'Helvetica Neue';
            display: inline-block;
            text-align: center;
        }

        h1 {
            font-size: 1.8rem;
            color: white;
        }

        i {
            font-size: 2.2rem;
        }

        p {
            font-size: 1.4rem;
        }

        .topnav {
            overflow: hidden;
            background-color: #0A1128;
        }

        body {
            margin: 0;
        }

        .content {
            padding: 5%;
        }

        .card-grid {
            max-width: 800px;
            margin: 0 auto;
            display: grid;
            grid-gap: 2rem;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
        }

        .input-group {
            display: flex;
        }

        .card {
            background-color: white;
            box-shadow: 2px 2px 12px 1px rgba(8, 8, 8, 0.5);
            border-radius: 10px;
        }

        .card-title {
            font-size: 1.2rem;
            font-weight: bold;
            color: #fefcfb;
        }

        input[type=submit] {
            border: none;
            color: #FEFCFB;
            background-color: #034078;
            padding: 15px 15px;
            text-align: center;
            text-decoration: none;
            display: inline-block;
            font-size: 16px;
            width: 100px;
            margin-right: 10px;
            border-radius: 10px;
            transition-duration: 0.4s;
        }

        input[type=submit]:hover {
            background-color: #1282A2;
        }

        input[type=text],
        input[type=number],
        select {
            width: 50%;
            padding: 12px 20px;
            margin: 18px;
            display: inline-block;
            border: 1px solid #ccc;
            border-radius: 10px;
            box-sizing: border-box;
        }

        .value {
            font-size: 1.2rem;
            color: #1282A2;
        }

        .state {
            font-size: 1.2rem;
            color: #1282A2;
        }

        button {
            border: none;
            color: #FEFCFB;
            padding: 15px 32px;
            text-align: center;
            font-size: 16px;
            width: 100px;
            border-radius: 10px;
            transition-duration: 0.4s;
        }

        .button-on {
            background-color: #034078;
        }

        .button-on:hover {
            background-color: #1282A2;
        }

        .button-off {
            background-color: #858585;
        }

        .button-off:hover {
            background-color: #252524;
        }

        .icon {
            font-size: xx-large;
        }

        .dhcp label {
            font-size: 1.2rem;
            width: 100px;
            display: inline-block;
            text-align: right;
        }

        .ota label {
            font-size: 1.2rem;
            width: 100px;
            display: inline-block;
            text-align: right;
        }

        .wificlient label {
            font-size: 1.2rem;
            width: 70px;
            display: inline-block;
            text-align: right;
        }

        .wificlient small {
            width: 60px;
            display: inline-block;
            text-align: right;
        }

        .mdns label {
            font-size: 1.2rem;
            width: 140px;
            display: inline-block;
            text-align: right;
        }

        .mdns small {
            width: 60px;
            display: inline-block;
            text-align: right;
        }

        /*input[type=checkbox] {
            padding: 1px 2px;
            margin-left: 10px;
            text-align: right;
            width: auto;
            display: inline-block;
        }*/

        input[type="checkbox"] {
            display: none;
        }

        input[type="checkbox"]+label {
            display: inline-block;
            position: relative;
            /*padding-left: 35px;*/
            margin-bottom: 20px;
            padding: 1px 2px;
            margin-left: 10px;
            text-align: right;
            font: 14px/20px "Open Sans", Arial, sans-serif;
            cursor: pointer;
            -webkit-user-select: none;
            -moz-user-select: none;
            -ms-user-select: none;
            user-select: none;
        }

        input[type="checkbox"]:hover+label:hover {
            color: rgb(23, 86, 228);
        }

        input[type="checkbox"]:hover+label:before {
            border: 1px solid #343a3f;
            box-shadow: 2px 1px 0 #343a3f;
        }

        input[type="checkbox"]+label:last-child {
            margin-bottom: 0;
        }

        input[type="checkbox"]+label:before {
            content: "";
            display: block;
            width: 1.4em;
            height: 1.4em;
            border: 1px solid #343a3f;
            border-radius: 0.2em;
            position: absolute;
            left: 0;
            top: 0;
            -webkit-transition: all 0.2s, background 0.2s ease-in-out;
            transition: all 0.2s, background 0.2s ease-in-out;
            background: rgba(255, 255, 255, 0.03);
            box-shadow: -2px -1px 0 #343a3f;
            background: #f3f3f3;
        }

        input[type="checkbox"]:checked+label:before {
            border: 2px solid #fff;
            border-radius: 0.3em;
            background: #16c60c;
            box-shadow: 2px 1px 0 #50565a;
        }

        input[type=button] {
            padding: 1px 2px;
            margin-left: 10px;
            text-align: right;
            border-radius: 6px;
            display: inline-block;
            width: auto;
            font-size: large;
            color: #1b0c8787;
        }
    </style>
</head>

<body>
    <noscript>
        <strong>We're sorry but the EasyNetwork Web Manager doesn't work properly without JavaScript enabled. Please
            enable it to continue.</strong>
    </noscript>
    <div class="topnav">
        <h1>Easy Network Manager Wifi Portal</h1>
    </div>
    <div class="content">
        <div class="card-title">
            <h3>
                <div class="icon">&#128246;</div>
                Connect to your network
            </h3>
        </div>
        <div class="card-grid">
            <div class="card">
                <form action="/wifimanager/builtin/wifi" method="POST">
                    <section class="wificlient">
                        <p>
                            <large>Wifi Router Details</large>
                            <br>
                        <div>
                            <small style="font-size: xx-large;">&#128462;</small>
                            <label for="ssid">SSID</label>
                            <input type="text" id="ssid" name="ssid" value="FakePSW" placeholder="your wifi name">
                            <br>
                        </div>
                        <div>
                            <small style="font-size: x-large;">&#128273;</small>
                            <label for="password">Password</label>
                            <input type="password" id="password" name="password" placeholder="your wifi password">
                            <br>
                        </div>
                        </p>
                    </section>
                    <hr>
                    <!-- <section class="dhcp">
                        <p>
                            <large>Wifi Client Details</large>
                            <br>
                            <br>
                            <input type="checkbox" id="dhcp" name="dhcp" onclick="enableDHCP()" unchecked><label
                                for="dhcp">Use DHCP</label>
                            <br>
                            <label for="ip">IP Address</label>
                            <input type="text" id="ip" name="ip" placeholder="192.168.1.12"><br>
                            <label for="gateway">Gateway</label>
                            <input type="text" id="gateway" name="gateway" placeholder="192.168.178.1"><br>
                            <label for="subnet">Subnet</label>
                            <input type="text" id="subnet" name="subnet" placeholder="255.255.255.0"><br>
                            <script>
                                enableDHCP();

                                function enableDHCP() {
                                    // Get the checkbox
                                    var dhcp_checkBox = document.getElementById("dhcp");

                                    // If the checkbox is checked
                                    if (dhcp_checkBox.checked == true) {
                                        document.getElementById("ip").disabled = false;
                                        document.getElementById("gateway").disabled = false;
                                        document.getElementById("subnet").disabled = false;
                                    } else {
                                        document.getElementById("ip").disabled = true;
                                        document.getElementById("gateway").disabled = true;
                                        document.getElementById("subnet").disabled = true;
                                    }
                                }
                            </script>
                        </p>
                    </section>
                    <hr>-->
                    <br>
                    <section class="mdns">
                        <p>
                            <large>MDNS Settings</large>
                            <br>
                            <br>
                            <input type="checkbox" id="mdns-check" name="mdns-check" onclick="enableMDNS()"
                                unchecked><label for="mdns-check">Use MulticastDNS</label>
                            <br>
                            <label for="url">mDNS URL</label><input type="button" id="url" name="url"><br>
                            <br>
                            <label for="mdns">mDNS Address </label>
                            <input type="text" id="mdns" name="mdns" placeholder="esp32device"
                                oninput='document.getElementById("url").value="http:/\/"+document.getElementById("mdns").value+".local";'><br>
                            <script>
                                document.getElementById("url").value = "http://" + document.getElementById("mdns").value + ".local";

                                enableMDNS();

                                function enableMDNS() {
                                    // Get the checkbox
                                    var mdns_checkBox = document.getElementById("mdns-check");

                                    // If the checkbox is checked
                                    if (mdns_checkBox.checked == true) {
                                        document.getElementById("url").disabled = false;
                                        document.getElementById("mdns").disabled = false;
                                    } else {
                                        document.getElementById("url").disabled = true;
                                        document.getElementById("mdns").disabled = true;
                                    }
                                }
                            </script>
                            <bold style="color:rgba(18, 14, 122, 0.751); font-size: large;">To see mDNS urls using an
                                Android device, please use the <ins style="color:#ff0e01;">Bonjour Browser</ins> App.
                            </bold>
                        </p>
                    </section>
                    <hr>
                    <br>
                    <section class="ota">
                        <p>
                            <large>OTA Settings</large>
                            <br>
                            <br>
                            <input type="checkbox" id="ota" name="ota" onclick="enableOTA()" unchecked><label
                                for="ota">Use OTA</label>
                            <br>
                            <label for="ota_password">Password</label>
                            <input type="text" id="ota_password" name="ota_password" placeholder="12345678"><br>
                            <label for="ota_port">OTA Port</label>
                            <input type="text" id="ota_port" name="ota_port" placeholder="3232"><br>
                            <script>
                                enableOTA();

                                function enableOTA() {
                                    // Get the checkbox
                                    var ota_checkBox = document.getElementById("ota");

                                    // If the checkbox is checked
                                    if (ota_checkBox.checked == true) {
                                        document.getElementById("ota_port").disabled = false;
                                        document.getElementById("ota_password").disabled = false;
                                    } else {
                                        document.getElementById("ota_port").disabled = true;
                                        document.getElementById("ota_password").disabled = true;
                                    }
                                }
                            </script>
                        </p>
                    </section>
                    <br>
                    <hr>
                    <p>
                        <!-- <input type="checkbox" id="reboot" name="reboot" checked><label for="reboot"
                            style="padding-left: 35px;">Reboot Device</label>
                        <br>
                        <br> -->
                        <input type="submit" value="Save">
                    </p>
                    <p>
                    </p>
                </form>
            </div>
        </div>
        <p>
            Advanced OTA Updates are coming soon - For now, use <ins style="color:#ff0e01;">PlatformIO's OTA</ins> to
            upload
            <br>
            <br>
            <!-- <br>
            <a href="/upload">
                <input type="submit" value="Upload">
            </a>
            <br> -->
        </p>
    </div>
</body>
`;

function chunkArray(myArray, chunk_size) {
    let index = 0;
    const arrayLength = myArray.length;
    const tempArray = [];
    for (index = 0; index < arrayLength; index += chunk_size) {
        myChunk = myArray.slice(index, index + chunk_size);
        // Do something if you want with the group
        tempArray.push(myChunk);
    }
    return tempArray;
}

function addLineBreaks(buffer) {
    let data = '';
    const chunks = chunkArray(buffer, 30);
    chunks.forEach((chunk, index) => {
        data += chunk.join(',');
        if (index + 1 !== chunks.length) {
            data += ',\n';
        }
    });
    return data;
}


gzip(HTML, { numiterations: 15 }, (err, output) => {
    if (err) {
        return console.error(err);
    }

    const FILE = `#ifndef EasyNetworkWebManager_h
#define EasyNetworkWebManager_h
#include <Arduino.h>
const uint32_t WEB_MANAGER_HTML_SIZE = ${output.length};
const uint8_t WEB_MANAGER_HTML[] PROGMEM = { 
${addLineBreaks(output)} 
};
#endif
`;

    FS.writeFileSync(path.resolve(__dirname, '../include/data/webpage.h'), FILE);
    console.log(`[COMPRESS] Compressed Build Files to webpage.h: ${(output.length / 1024).toFixed(2)}KB`);
});