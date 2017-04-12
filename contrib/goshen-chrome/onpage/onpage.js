// This is run inside the scope of a page, and so we have direct access to the
// page's HTML from here.

(function (window) {

    if (typeof window._goshen !== 'undefined') {
        console.warn("_goshen unable to initialize!");
        return;
    } else {
        window._goshen = {};
    }

    // We can now request the contents of window.

    window.addEventListener('keyup', function(ev) {
        // This is a bit heavy-handed, and we almost assuredly don't need to be
        // capturing every keyup event. But it's lightweight, and serves as a
        // decent proof of concept.
        if (ev.altKey && ev.keyCode == 84) {
            // They pressed Alt+T. Call _goshen's get-text function!
            window._goshen._cg.selectMode();
        }
    });

})(this);
