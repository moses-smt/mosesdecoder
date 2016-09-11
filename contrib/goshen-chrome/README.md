# goshen

Goshen is a Chrome extension that duplicates the utility of the Google Translate chrome extension for on-page website translation, using the Goshen JavaScript library with Moses as a backend translator. (It also has the ability to swap in an arbitrary translation engine, if the appropriate adapters are written.)


## 1. The Goshen.js Library
As Google Translate is the current go-to machine-translation system for developers, I intend to make Moses a viable alternative for even the non-savvy developer. This is in large part simplified by having an easily deployed (perhaps Dockerized) Moses server, as mentioned in the section above. However, it is also greatly simplified by exposing a comprehensive and well-formed JavaScript API that allows the same level of flexibility as the existing Google API.

Instead of trying to duplicate the Google Translate API, I instead chose to write a wrapper for *any* translation engine. An engine with an exposed HTTP endpoint can be added to the Goshen translation library by implementing `GoshenAdapter`, for which I have provided a complete `moses-mt-server` implementation (`MosesGoshenAdapter`) and a partially complete proof of concept for Google Translate (`GoogleTranslateGoshenAdapter`). This is to illustrate that the engines can be used interchangeably for simple translation tasks, but the entirety of Moses functionality can be accessed whereas Google Translate's public API fails to accommodate some more technical tasks.

The library is both commented and minified, available in the `goshenlib/` directory, [here](https://github.com/j6k4m8/goshen-moses). It is also possible to import the unminified, importable version from `goshenlib/dist`. The complete documentation, as well as usage examples and implementation explanations and justifications, are available in `goshenlib/docs` at the above repository.

## 2. Chrome Extension
This directory contains a Chrome extension that utilizes the CASMACAT moses-mt-server/Moses backend to provide a frontend website translation service. The extension automatically detects the relevant content of most articles or body-text on the page, and at the user's request, translates it to the requested language. Usage is explained below, as well as inside the extension popup after installation, for quick reference.

### Usage
1. **Install the unpacked extension.** Go to `chrome://extensions` and click <kbd>Load Unpacked Extension</kbd>. Navigate to this `goshen-chrome/` directory, and load.
2. This adds a Goshen icon to your Chrome toolbar. Clicking it brings up a simple modal that allows the switching of languages.
3. Use the <kbd>Alt</kbd>+<kbd>T</kbd> key-chord ("T" for "Translate") to begin text-selection. The Goshen-translate extension will highlight elements of text in cyan as you mouse over them: To translate what is currently highlighted, click.

For more information, see [this full report](https://github.com/j6k4m8/goshen-moses/blob/master/report/report.md), or contact Jordan Matelsky (@j6k4m8).
