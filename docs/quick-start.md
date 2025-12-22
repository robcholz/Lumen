# Quick Start

Goal of this guide:
**Get Lumen running without setting up any local development environment.**

You do not need to understand embedded systems or modify any code.

---

> Before you start
>
> This quick start assumes you **already have Lumen hardware**.
>
> If you do not have the hardware yet, you can make it through **JLCPCB**:
>
> - PCB: upload the Gerber files and place an SMT order
> - Enclosure: download the STL files and 3D print
>
> All fabrication files are ready in:
> **/hardware**

<details>
<summary><strong>No Lumen hardware yet? (Click to view build steps)</strong></summary>

### Hardware build steps (JLCPCB)

#### 1. PCB fabrication and SMT

1. Open the JLCPCB order page
2. Upload the **Gerber(gerber.zip)** file from the repository
3. PCB thickness 1.2mm, copper 1-2oz, 2-layer, black
4. Choose **SMT assembly**
5. Upload **BOM(hardware/bom.csv)** and **Pick & Place(hardware/pickandplace.csv)**
6. Place the order with default process settings

#### 2. Enclosure 3D printing

1. Download the **STL files** from the repository
2. Use any 3D printing service or local printer
3. Recommended material: PLA / PETG, black filament, 0.10mm precision
4. Default printing settings should fit correctly

#### 3. Panel fabrication

1. Use 1/16 inch clear acrylic
2. Cut and **engrave both sides**; engraving depth is noted in `top-cover.ai`
3. For the light guide (`light-guide.ai`), sand the four corners

#### 4. Display

[Aliexpress Product Link](https://www.aliexpress.us/item/3256808537118373.html)

Or keyword:
`DIYTZT 1.54 Inch 1.54" Full Color TFT Display Module HD IPS LCD LED Screen 240x240 SPI Interface ST7789 For Arduino`

#### 5. Assembly

1. Solder the PCB wires to the display using thin wire; keep orientation consistent with the render
2. Install the PCB reversed into `bottom-shell.stl`, then use 502 glue or M1.3 tapping screws for `side-shell.stl`
3. Install the light guide and panel with 502 glue

</details>

## What you need

- A Lumen device
- A USB-C data cable
- A computer with Chrome or Edge

## Step 1: Connect the device

Connect Lumen to your computer with a USB-C cable.

On first connection, your system may report a new USB device. This is normal.

## Step 2: Open the web flasher

Open this page:

[**Web Flasher**](https://web.esphome.io/)

This is a browser-based flashing tool.
No software installation required.

## Step 3: Flash the firmware

1. Click **Connect**
2. Select Lumen in the popup dialog
3. Download the latest `lumen_single_bin_{version}.bin` from Releases
4. Choose the downloaded firmware and click **Flash**

The whole process usually takes less than a minute.

## Step 4: Wait for reboot

After flashing, Lumen will reboot automatically.

If everything is fine, you should see:

- The screen lights up
- The UI displays normally
- The knob or controls respond

Done.

## What next?

- Read the UI guide
- Try different interactions

## UI controls

- Turn the knob: browse screens
- Short press: select / enter
- Long press: exit current screen
