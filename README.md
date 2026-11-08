# PSoC Labs

Laboratory work for the course **"Programming of Systems on a Chip"** (Uzhhorod National University,
Faculty of Engineering and Technology, Department of Computer Systems and Networks).

Every lab is a separate folder with its own report (`README.md`), one PSoC Creator project per task and
a `.gitignore` that keeps build artifacts out of the repository.

| Folder | Topic | Report |
|--------|-------|--------|
| [`lab01_Hutsul`](lab01_Hutsul/) | Clock tree configuration and interrupt service routines (ISR) in PSoC 4 | [README](lab01_Hutsul/README.md) |
| [`lab02_Hutsul`](lab02_Hutsul/) | Asynchronous serial communication over UART (SCB): command-line console, Saleae Logic measurements | [README](lab02_Hutsul/README.md) |

## Hardware

* **CY8CKIT-042 PSoC 4 Pioneer Kit** with the **CY8C4245AXI-483** microcontroller (ARM Cortex-M0, 48 MHz).
  The kit has an on-board KitProg programmer/debugger, an RGB LED (P0[2] green, P0[3] blue, P1[6] red)
  and a user push button SW2 on P0[7].
* **Saleae Logic** logic analyzer (optional) for measuring the generated signals.

## Software

* **PSoC Creator 4.4** – free IDE from Infineon (Cypress).
  Download: <https://www.infineon.com/cms/en/design-support/tools/sdk/psoc-software/psoc-creator/>
  (a free Infineon account is required). Run the installer with the default options; it includes the
  GCC ARM toolchain and the KitProg drivers.
* **Saleae Logic 2** (optional): <https://www.saleae.com/downloads/>.

## How to build and run a project

1. Clone the repository. Keep the path **ASCII-only** (PSoC Creator refuses workspaces whose path contains
   non-Latin characters).
2. Start PSoC Creator and open the workspace file of the task you need, e.g.
   `lab01_Hutsul/task1/lab1.cywrk` (**File → Open → Project/Workspace**).
3. The `Generated_Source` folder is not stored in Git. Press **Build → Build lab1** (`Shift+F6`);
   PSoC Creator regenerates all component APIs and compiles the firmware. The Output window must end with
   `Build Succeeded`.
4. Connect the CY8CKIT-042 kit over USB and press **Debug → Program** (`Ctrl+F5`).
   If the programmer is not detected, run **Debug → Select Debug Target** and choose *KitProg*.
5. Each lab README describes what to expect on the board and how to measure the signals.

## Repository rules

* Only the source files of each project are committed: `*.cywrk`, `*.cyprj`, `*.cydwr`,
  `TopDesign/TopDesign.cysch`, `main.c` and other hand-written sources.
* Build output (`CortexM0/`, `Generated_Source/`, `codegentemp/`, reports) is ignored via `.gitignore`.
* Every lab starts with a `lab_x initial commit` containing the untouched PSoC Creator template;
  the student's code is added in the following commits.
