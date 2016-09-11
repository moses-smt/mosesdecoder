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

## Goshen.js Documentatio

### Overview
The Goshen library provides a web-developer-facing library for handling machine translation. It allows interaction with arbitrary machine translation services, agnostic of the technology or algorithm stack.

### Usage
A very brief tutorial is provided here:

- Create a new Goshen object. Use the MosesGoshenAdapter, so that translations are handled by a Moses MT server.
    ```JavaScript
    g = new Goshen('localhost:3000', 'http', MosesGoshenAdapter);
    ```
- Use the Goshen object to pass a translation job to the Moses adapter. The adapter will pass back a completed translation once the job completes.
    ```JavaScript
    g.translate('This is a simple sentence.', Languages.ENGLISH, Languages.SPANISH);
    ```
- You can also optionally pass a callback function to the .translate method:
    ```JavaScript
    g.translate('This is a simple sentence.',
                Languages.ENGLISH,
                Languages.SPANISH,
                function(err, val) {
        if (!!err) {
            console.warn("Encountered an error: " + err);
        } else {
            console.info("Translated to: " + val);
        }
    });
    ```
    If a callback is supplied, the function is run on a new thread, and is non-blocking. If one is not supplied, then the return value of the function contains the translated text. `undefined` is returned if the translation fails.


### `Goshen`
The generic class for a Goshen.js object, the object that handles translation with an arbitrary translation backend. In order to specify a backend, pass a `type` parameter to the constructor. (Default is Moses, of course!)

- `Goshen`
    - Arguments:
        - `hostname`: A string hostname, such as `locahost:8000`. This is the base URL for formulating the RESTful API endpoint.
        - `protocol`: The HTTP protocol. Either `http` or `https`.
        - `type`: What type of GoshenAdapter to use. Options are currently `GoogleTranslateGoshenAdapter` or `MosesGoshenAdapter`.
        - `opts`: A dictonary of options to pass to the adapter constructor. Currently, none are required for existing adapters.

- function `url`

    Generate a complete URI. If `hostname` is `localhost:8000` and `protocol` is `https`, then `this.url('foo')` returns `https://localhost:8000/foo`
    - Arguments:
        - `suffix`: A suffix to concatenate onto the end of a well-formed URI.
    - Returns:
        - String: The complete web-accessible URL.

- function `translate`

    Translate a text from a source language to a target language.
    - Arguments:
        - `text`: The text to translate. If this is too long, a series of truncated versions are translated, splitting on sentence-delimiters if possible.
        - `source`: An item from the `LANGUAGES` set (e.g. `'en-us'`)
        - `target`: An item from the `LANGUAGES` set (e.g. `'en-us'`)
        - `callback`: Optional. If supplied, must be a function (or be of a callable type) that will be run with `errors` and `value` as its two arguments.
    - Returns:
        - String: The translated text. All supplementary data, such as alignments or language detections, are ignored by this function.


### `GoshenAdapter`
The `Goshen` class secretly outsources all of its computation to a GoshenAdapter class attribute, which is responsible for performing the machine translation. `GoshenAdapter`s should expose `url` and `translate` functions unambiguously, with the same signatures as those in the `Goshen` class. Other functions may be optionally exposed.

#### `MosesGoshenAdapter`
This is one particular implementation of the `GoshenAdapter` type, that uses the `moses-mt-server` backend as its translation engine API endpoint. It splits text into manageable chunks when translating, to avoid crashing the underlying Moses server (RAM allocation fail).

#### `GoogleTranslateGoshenAdapter`
This is another implementation of the `GoshenAdapter` type, that uses the Google Translate API as its translation engine endpoint. Because Google handles arbitrarily long text, this adapter does not split text, as `MosesGoshenAdapter`s do.


For more information, see [this full report](https://github.com/j6k4m8/goshen-moses/blob/master/report/report.md), or contact Jordan Matelsky (@j6k4m8).
