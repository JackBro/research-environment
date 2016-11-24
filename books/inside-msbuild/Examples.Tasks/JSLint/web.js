// web.js
// 2008-03-15

// This is the web browser companion to fulljslint.js.

/*extern JSLINT */
/*jslint browser: true, evil: true */

(function () {
    var c = document.cookie,
        cluster = {
            recommended: ['eqeqeq', 'nomen', 'undef', 'white'],
            goodparts: [
                'bitwise', 'eqeqeq', 'nomen', 'plusplus', 'regexp', 'undef',
                'white'
            ],
            clearall: []
        },
        i,                              // Loop counter
        input = document.getElementById('input'),
        n,                              // A dom node
        ns,                             // An array of dom nodes
        o,                              // The options object
        options = [
            'adsafe', 'bitwise', 'browser', 'cap', 'debug', 'eqeqeq',
            'evil', 'forin', 'fragment', 'laxbreak', 'nomen', 'on', 'passfail',
            'plusplus', 'regexp', 'rhino', 'sidebar', 'undef', 'white', 'widget'
        ],
        output = document.getElementById('output');

    function getOption(o) {
        var n = document.getElementById(o);
        return n && n.checked;
    }


    function setOption(o, b) {
        var n = document.getElementById(o);
        if (n) {
            n.checked = b;
        }
    }

    function setCluster(n) {
        document.getElementById(n).onclick = function (e) {
            var c = cluster[n];
            for (i = 0; i < options.length; i += 1) {
                setOption(options[i], false);
            }
            for (i = 0; i < c.length; i += 1) {
                setOption(c[i], true);
            }
        };
    }


    input.onchange = function (e) {
        output.innerHTML = '';
    };

// Add click event handlers to the [JSLint] and [clear] buttons.

    ns = document.getElementsByName('jslint');
    for (i = 0; i < ns.length; i += 1) {
        n = ns[i];
        switch (n.value) {
        case 'JSLint':
            n.onclick = function (e) {

// Make a JSON cookie of the current options.

                var b, d = new Date(), j, k = '{', oj, op = {}, v;
                for (j = 0; j < options.length; j += 1) {
                    oj = options[j];
                    v = getOption(oj);
                    op[oj] = v;
                    if (b) {
                        k += ',';
                    }
                    k += '"' + oj + '":' + v;
                    b = true;
                }
                k += '}';
                d.setTime(d.getTime() + 1e10);
                document.cookie = 'jslint=' + k + ';expires=' +
                    d.toGMTString();

// Call JSLint and obtain the report.

                JSLINT(input.value, op);
                output.innerHTML = JSLINT.report();
                input.select();
                return false;
            };
            break;
        case 'clear':
            n.onclick = function (e) {
                input.value = '';
                output.innerHTML = '';
                input.select();
                return false;
            };
            break;
        }
    }

// Recover the JSLint options from a JSON cookie.

    if (c && c.length > 8) {
        i = c.indexOf('jslint={');
        if (i >= 0) {
            c = c.substring(i + 7);
            i = c.indexOf('}');
            if (i > 1) {
                c = c.substring(0, i + 1);
                o = eval('(' + c + ')');
                for (i = 0; i < options.length; i += 1) {
                    setOption(options[i], o[options[i]]);
                }
            }
        }
    }

    setCluster('recommended');
    setCluster('goodparts');
    setCluster('clearall');

    input.select();
})();