(function (root) {

    var _goshen = root._goshen;

    LANGUAGES = {
        English: 'en',
        en: 'en',
        German: 'de',
        de: 'de'
    }

    LOCALES = {
        English: 'en-US',
        en: 'en-US',
        German: 'de',
        de: 'de'
    }


    serialize = function(obj) {
        var str = [];
        for (var p in obj) {
            str.push(encodeURIComponent(p) + "=" + encodeURIComponent(obj[p]));
        }
        return str.join("&");
    };


    class MosesGoshenAdapter {
        constructor(hostname, protocol, opts) {
            this.hostname = hostname;
            this.protocol = protocol || 'http';
        }

        url(suffix) {
            suffix = suffix || '';
            return `${this.protocol}://${this.hostname}/translate?${suffix}`;
        }

        translate(text, target, source, callback) {
            /* Translate a string `text`, using `opts` as corequisite options.

            Arguments:
            text (str): The text to translate.
            target (str): The language to translate to
            source (str): The language to translate from
            callback (function): The function to call on the translated text

            Returns:
            str: The translated text
            */

            var requestURL = this.url(serialize({
                q: text,
                key: 'x',
                target: target || LANGUAGES.en,
                source: source || LANGUAGES.de
            }));

            if (!!root.Meteor && !!root.HTTP) {
                var response = HTTP.call('GET', requestURL, {});
                var translated = response.data;
                if (callback) callback(text, translated);

            } else if (!!root.XMLHttpRequest) {
                var request = new XMLHttpRequest();
                request.open('GET', requestURL, false);
                request.send(null);

                if (request.status === 200) {
                    var translated = root.JSON.parse(request.responseText);
                    if (callback) callback(text, translated);
                }
            }
            return translated.data.translations[0].translatedText
        }
    }


    _goshen.Goshen = class Goshen {
        constructor(hostname, protocol, type, opts) {
            /* Create a new Goshen object.

            Arguments:
            hostname (str): A protocol-less URI such as `255.255.0.0:3000`
            protocol (str: 'http'): An http protocol (either 'http' or 'https')
            type (class): The type of adapter to use by default.
            opts (dict): Options for configuration.

            The options configuration dictionary can contain
            */
            type = type || MosesGoshenAdapter;
            this.ga = new type(hostname, protocol, opts);
        }

        url(suffix) {
            return this.ga.url(suffix);
        }

        translate(text, target, source, callback) {
            /* Calls the local GoshenAdapter#translate. */
            return this.ga.translate(text, target, source, callback);
        }


    };
})(this);
