(function(window) {

    var demo_url = "ec2-52-23-242-15.compute-1.amazonaws.com:8081";

    var _goshen = window._goshen;

    on = function(event, cb) {
        window.addEventListener(event, cb);
    }

    off = function(event, cb) {
        window.removeEventListener(event, cb);
    }

    class ChromeGoshen {
        constructor() {
            this.G = new _goshen.Goshen(demo_url);
            console.info("Goshenjs engine loaded successfully.")
        }

        /**
        * Begin interactive dom node selection.
        */
        selectMode() {
            var self = this;
            var selection = [];
            var previousElement = null;

            var showSelection = function() {
                var olds = document.querySelectorAll('._goshen-selected');
                for (var i = 0; i < olds.length; i++) {
                    olds[i].classList.remove('_goshen-selected');
                }
                for (var i = 0; i < selection.length; i++) {
                    selection[i].classList.add('_goshen-selected');
                }
            };

            var setSelection = function(sel) {
                selection = sel;
                showSelection();
            };

            var validParents = [
                "DIV", "ARTICLE", "BLOCKQUOTE", "MAIN",
                "SECTION", "UL", "OL", "DL"
            ];

            var validChildren = [
                "P", "H1", "H2", "H3", "H4", "H5", "H6", "SPAN", "DL",
                "OL", "UL", "BLOCKQUOTE", "SECTION"
            ];

            var selectSiblings = function(el) {
                var firstChild = el;
                var parent = el.parentNode;
                while (parent && !~validParents.indexOf(parent.tagName)) {
                    firstChild = parent;
                    parent = firstChild.parentNode;
                }

                if (parent) {
                    var kids = parent.childNodes,
                        len = kids.length,
                        result = [],
                        i = 0;

                    while (kids[i] !== firstChild) { i++; }

                    for (; i < len; i++) {
                        var kid = kids[i];
                        if (!!~validChildren.indexOf(kid.tagName)) {
                            result.push(kid);
                        }
                    }
                    return result;

                } else { return [el]; }
            };

            var stop = function() {
                off("mouseover", mouseoverHandler);
                off("mousemove", moveHandler);
                off("keydown", keydownHandler);
                off("keyup", keyupHandler);
                off("click", clickHandler);
                self.performSelectTranslation(selection);
            };

            var mouseoverHandler = function(ev) {
                previousElement = ev.target;

                if (ev.altKey) {
                    setSelection([ev.target]);
                } else {
                    setSelection(selectSiblings(ev.target));
                }
            };

            var clickHandler = function(ev) {
                stop();
            };

            var moveHandler = function(ev) {
                mouseoverHandler(ev);
                off("mousemove", moveHandler);
            };

            var keydownHandler = function(ev) {
                if (ev.keyCode === 27) {
                    stop();
                } else if (ev.altKey && selection.length > 1) {
                    setSelection([selection[0]]);
                }
            };

            var keyupHandler = function(ev) {
                if (!ev.altKey && selection.length === 1) {
                    setSelection(selectSiblings(selection[0]));
                }
            };

            on("mouseover", mouseoverHandler);
            on("click", clickHandler);
            on("mousemove", moveHandler);
            on("keydown", keydownHandler);
            on("keyup", keyupHandler);
        }

        select(contextData) {
            var text;
            if (contextData === undefined) {
                text = window.getSelection().toString();
            } else {
                text = contextData.selectionText;
            }
            if (text.trim().length > 0) {
                this.init(this.parse.string(text));
                window.getSelection().removeAllRanges();
            } else {
                selectMode();
            }
        };

        _chunkedTranslation(text) {
            // We need to find a way to split on sentences, or long things.
            var texts = text.split('.');
            for (var i = 0; i < texts.length; i++) {
                texts[i] = this.G.translate(texts[i]);
            }
            return texts.join('.');
        }

        performSelectTranslation(selection) {
            for (var i = 0; i < selection.length; i++) {
                selection[i].classList.add('_goshen-active');
                selection[i].innerText = this._chunkedTranslation(selection[i].innerText);
                selection[i].classList.remove('_goshen-active');
                selection[i].classList.remove('_goshen-selected');
            }
        }
    };

    _goshen._cg = new ChromeGoshen();

})(this);
