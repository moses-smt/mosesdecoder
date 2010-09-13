// file: frameset.js

// Herve Saint-Amand
// Universitaet des Saarlandes
// Tue May 13 10:00:42 2008

//-----------------------------------------------------------------------------

function startCount () {
    var startTime = (new Date()).getTime ();
    var element   = document.getElementById ('status');

    function step () {
        var secs = parseInt (((new Date()).getTime() - startTime) / 1000);
        var status = "(elapsed: " + secs + " seconds)";
        if (top.numSentences != null)
            status += " (" + top.numSentences + " segments)";
        else
            setTimeout (step, 1000);
        element.innerHTML = status;
    }

    step ();
}

//-----------------------------------------------------------------------------
