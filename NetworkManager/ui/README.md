# UI Source Code

This is the source code for generating the `wifimanager` UI, and the `OTA` UI (coming soon).

## Building

To build the UI, you need to have `node` and `yarn` installed. Then, run the following commands:

    yarn
    yarn build

This will generate the UI in the `include/data` folder as `webpage.h`.

## Development

To develop the UI, currently we use raw html, css, and javascript. Simply modify the `html` files and copy the file to the string `string` in `compress.js`.

This is only temporary, and we will be using a more robust framework in the future.
