//We have to do this here so the YAHOO js doesn't get upset
var navigator = [];
navigator["userAgent"]='';

/*
Copyright (c) 2008, Yahoo! Inc. All rights reserved.
Code licensed under the BSD License:
http://developer.yahoo.net/yui/license.txt
version: 2.5.1
*/
if(typeof YAHOO=="undefined"||!YAHOO){var YAHOO={};}YAHOO.namespace=function(){var A=arguments,E=null,C,B,D;for(C=0;C<A.length;C=C+1){D=A[C].split(".");E=YAHOO;for(B=(D[0]=="YAHOO")?1:0;B<D.length;B=B+1){E[D[B]]=E[D[B]]||{};E=E[D[B]];}}return E;};YAHOO.log=function(D,A,C){var B=YAHOO.widget.Logger;if(B&&B.log){return B.log(D,A,C);}else{return false;}};YAHOO.register=function(A,E,D){var I=YAHOO.env.modules;if(!I[A]){I[A]={versions:[],builds:[]};}var B=I[A],H=D.version,G=D.build,F=YAHOO.env.listeners;B.name=A;B.version=H;B.build=G;B.versions.push(H);B.builds.push(G);B.mainClass=E;for(var C=0;C<F.length;C=C+1){F[C](B);}if(E){E.VERSION=H;E.BUILD=G;}else{YAHOO.log("mainClass is undefined for module "+A,"warn");}};YAHOO.env=YAHOO.env||{modules:[],listeners:[]};YAHOO.env.getVersion=function(A){return YAHOO.env.modules[A]||null;};YAHOO.env.ua=function(){var C={ie:0,opera:0,gecko:0,webkit:0,mobile:null,air:0};var B=navigator.userAgent,A;if((/KHTML/).test(B)){C.webkit=1;}A=B.match(/AppleWebKit\/([^\s]*)/);if(A&&A[1]){C.webkit=parseFloat(A[1]);if(/ Mobile\//.test(B)){C.mobile="Apple";}else{A=B.match(/NokiaN[^\/]*/);if(A){C.mobile=A[0];}}A=B.match(/AdobeAIR\/([^\s]*)/);if(A){C.air=A[0];}}if(!C.webkit){A=B.match(/Opera[\s\/]([^\s]*)/);if(A&&A[1]){C.opera=parseFloat(A[1]);A=B.match(/Opera Mini[^;]*/);if(A){C.mobile=A[0];}}else{A=B.match(/MSIE\s([^;]*)/);if(A&&A[1]){C.ie=parseFloat(A[1]);}else{A=B.match(/Gecko\/([^\s]*)/);if(A){C.gecko=1;A=B.match(/rv:([^\s\)]*)/);if(A&&A[1]){C.gecko=parseFloat(A[1]);}}}}}return C;}();(function(){YAHOO.namespace("util","widget","example");if("undefined"!==typeof YAHOO_config){var B=YAHOO_config.listener,A=YAHOO.env.listeners,D=true,C;if(B){for(C=0;C<A.length;C=C+1){if(A[C]==B){D=false;break;}}if(D){A.push(B);}}}})();YAHOO.lang=YAHOO.lang||{isArray:function(B){if(B){var A=YAHOO.lang;return A.isNumber(B.length)&&A.isFunction(B.splice);}return false;},isBoolean:function(A){return typeof A==="boolean";},isFunction:function(A){return typeof A==="function";},isNull:function(A){return A===null;},isNumber:function(A){return typeof A==="number"&&isFinite(A);},isObject:function(A){return(A&&(typeof A==="object"||YAHOO.lang.isFunction(A)))||false;},isString:function(A){return typeof A==="string";},isUndefined:function(A){return typeof A==="undefined";},hasOwnProperty:function(A,B){if(Object.prototype.hasOwnProperty){return A.hasOwnProperty(B);}return !YAHOO.lang.isUndefined(A[B])&&A.constructor.prototype[B]!==A[B];},_IEEnumFix:function(C,B){if(YAHOO.env.ua.ie){var E=["toString","valueOf"],A;for(A=0;A<E.length;A=A+1){var F=E[A],D=B[F];if(YAHOO.lang.isFunction(D)&&D!=Object.prototype[F]){C[F]=D;}}}},extend:function(D,E,C){if(!E||!D){throw new Error("YAHOO.lang.extend failed, please check that "+"all dependencies are included.");}var B=function(){};B.prototype=E.prototype;D.prototype=new B();D.prototype.constructor=D;D.superclass=E.prototype;if(E.prototype.constructor==Object.prototype.constructor){E.prototype.constructor=E;}if(C){for(var A in C){D.prototype[A]=C[A];}YAHOO.lang._IEEnumFix(D.prototype,C);}},augmentObject:function(E,D){if(!D||!E){throw new Error("Absorb failed, verify dependencies.");}var A=arguments,C,F,B=A[2];if(B&&B!==true){for(C=2;C<A.length;C=C+1){E[A[C]]=D[A[C]];}}else{for(F in D){if(B||!E[F]){E[F]=D[F];}}YAHOO.lang._IEEnumFix(E,D);}},augmentProto:function(D,C){if(!C||!D){throw new Error("Augment failed, verify dependencies.");}var A=[D.prototype,C.prototype];for(var B=2;B<arguments.length;B=B+1){A.push(arguments[B]);}YAHOO.lang.augmentObject.apply(this,A);},dump:function(A,G){var C=YAHOO.lang,D,F,I=[],J="{...}",B="f(){...}",H=", ",E=" => ";if(!C.isObject(A)){return A+"";}else{if(A instanceof Date||("nodeType" in A&&"tagName" in A)){return A;}else{if(C.isFunction(A)){return B;}}}G=(C.isNumber(G))?G:3;if(C.isArray(A)){I.push("[");for(D=0,F=A.length;D<F;D=D+1){if(C.isObject(A[D])){I.push((G>0)?C.dump(A[D],G-1):J);}else{I.push(A[D]);}I.push(H);}if(I.length>1){I.pop();}I.push("]");}else{I.push("{");for(D in A){if(C.hasOwnProperty(A,D)){I.push(D+E);if(C.isObject(A[D])){I.push((G>0)?C.dump(A[D],G-1):J);}else{I.push(A[D]);}I.push(H);}}if(I.length>1){I.pop();}I.push("}");}return I.join("");},substitute:function(Q,B,J){var G,F,E,M,N,P,D=YAHOO.lang,L=[],C,H="dump",K=" ",A="{",O="}";for(;;){G=Q.lastIndexOf(A);if(G<0){break;}F=Q.indexOf(O,G);if(G+1>=F){break;}C=Q.substring(G+1,F);M=C;P=null;E=M.indexOf(K);if(E>-1){P=M.substring(E+1);M=M.substring(0,E);}N=B[M];if(J){N=J(M,N,P);}if(D.isObject(N)){if(D.isArray(N)){N=D.dump(N,parseInt(P,10));}else{P=P||"";var I=P.indexOf(H);if(I>-1){P=P.substring(4);}if(N.toString===Object.prototype.toString||I>-1){N=D.dump(N,parseInt(P,10));}else{N=N.toString();}}}else{if(!D.isString(N)&&!D.isNumber(N)){N="~-"+L.length+"-~";L[L.length]=C;}}Q=Q.substring(0,G)+N+Q.substring(F+1);}for(G=L.length-1;G>=0;G=G-1){Q=Q.replace(new RegExp("~-"+G+"-~"),"{"+L[G]+"}","g");}return Q;},trim:function(A){try{return A.replace(/^\s+|\s+$/g,"");}catch(B){return A;}},merge:function(){var D={},B=arguments;for(var C=0,A=B.length;C<A;C=C+1){YAHOO.lang.augmentObject(D,B[C],true);}return D;},later:function(H,B,I,D,E){H=H||0;B=B||{};var C=I,G=D,F,A;if(YAHOO.lang.isString(I)){C=B[I];}if(!C){throw new TypeError("method undefined");}if(!YAHOO.lang.isArray(G)){G=[D];}F=function(){C.apply(B,G);};A=(E)?setInterval(F,H):setTimeout(F,H);return{interval:E,cancel:function(){if(this.interval){clearInterval(A);}else{clearTimeout(A);}}};},isValue:function(B){var A=YAHOO.lang;return(A.isObject(B)||A.isString(B)||A.isNumber(B)||A.isBoolean(B));}};YAHOO.util.Lang=YAHOO.lang;YAHOO.lang.augment=YAHOO.lang.augmentProto;YAHOO.augment=YAHOO.lang.augmentProto;YAHOO.extend=YAHOO.lang.extend;YAHOO.register("yahoo",YAHOO,{version:"2.5.1",build:"984"});

/*
Copyright (c) 2008, Yahoo! Inc. All rights reserved.
Code licensed under the BSD License:
http://developer.yahoo.net/yui/license.txt
version: 2.5.1
*/
YAHOO.namespace("lang");YAHOO.lang.JSON={_ESCAPES:/\\["\\\/bfnrtu]/g,_VALUES:/"[^"\\\n\r]*"|true|false|null|-?\d+(?:\.\d*)?(?:[eE][+\-]?\d+)?/g,_BRACKETS:/(?:^|:|,)(?:\s*\[)+/g,_INVALID:/^[\],:{}\s]*$/,_SPECIAL_CHARS:/["\\\x00-\x1f\x7f-\x9f]/g,_PARSE_DATE:/^(\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2})Z$/,_CHARS:{"\b":"\\b","\t":"\\t","\n":"\\n","\f":"\\f","\r":"\\r",'"':'\\"',"\\":"\\\\"},_applyFilter:function(C,B){var A=function(E,D){var F,G;if(D&&typeof D==="object"){for(F in D){if(YAHOO.lang.hasOwnProperty(D,F)){G=A(F,D[F]);if(G===undefined){delete D[F];}else{D[F]=G;}}}}return B(E,D);};if(YAHOO.lang.isFunction(B)){A("",C);}return C;},isValid:function(A){if(!YAHOO.lang.isString(A)){return false;}return this._INVALID.test(A.replace(this._ESCAPES,"@").replace(this._VALUES,"]").replace(this._BRACKETS,""));},dateToString:function(B){function A(C){return C<10?"0"+C:C;}return'"'+B.getUTCFullYear()+"-"+A(B.getUTCMonth()+1)+"-"+A(B.getUTCDate())+"T"+A(B.getUTCHours())+":"+A(B.getUTCMinutes())+":"+A(B.getUTCSeconds())+'Z"';},stringToDate:function(B){if(this._PARSE_DATE.test(B)){var A=new Date();A.setUTCFullYear(RegExp.$1,(RegExp.$2|0)-1,RegExp.$3);A.setUTCHours(RegExp.$4,RegExp.$5,RegExp.$6);return A;}},parse:function(s,filter){if(this.isValid(s)){return this._applyFilter(eval("("+s+")"),filter);}throw new SyntaxError("parseJSON");},stringify:function(C,K,F){var E=YAHOO.lang,H=E.JSON,D=H._CHARS,A=this._SPECIAL_CHARS,B=[];var I=function(N){if(!D[N]){var J=N.charCodeAt();D[N]="\\u00"+Math.floor(J/16).toString(16)+(J%16).toString(16);}return D[N];};var M=function(J){return'"'+J.replace(A,I)+'"';};var L=H.dateToString;var G=function(J,T,R){var W=typeof J,P,Q,O,N,U,V,S;if(W==="string"){return M(J);}if(W==="boolean"||J instanceof Boolean){return String(J);}if(W==="number"||J instanceof Number){return isFinite(J)?String(J):"null";}if(J instanceof Date){return L(J);}if(E.isArray(J)){for(P=B.length-1;P>=0;--P){if(B[P]===J){return"null";}}B[B.length]=J;S=[];if(R>0){for(P=J.length-1;P>=0;--P){S[P]=G(J[P],T,R-1)||"null";}}B.pop();return"["+S.join(",")+"]";}if(W==="object"){if(!J){return"null";}for(P=B.length-1;P>=0;--P){if(B[P]===J){return"null";}}B[B.length]=J;S=[];if(R>0){if(T){for(P=0,O=0,Q=T.length;P<Q;++P){if(typeof T[P]==="string"){U=G(J[T[P]],T,R-1);if(U){S[O++]=M(T[P])+":"+U;}}}}else{O=0;for(N in J){if(typeof N==="string"&&E.hasOwnProperty(J,N)){U=G(J[N],T,R-1);if(U){S[O++]=M(N)+":"+U;}}}}}B.pop();return"{"+S.join(",")+"}";}return undefined;};F=F>=0?F:1/0;return G(C,K,F);}};YAHOO.register("json",YAHOO.lang.JSON,{version:"2.5.1",build:"984"});






(function () {
    try
    {
        var startDelim = '__JSLINT__START__';
        var endDelim = '__JSLINT__END__';


        var maxIterationCount = 1000;
        var currentInterationCount = 0;
        var strRead = '';
        var linesRead = 0;
        var foundStart = false;
        var foundEnd = false;
        
        
        //WScript.Quit(1);
        WScript.StdErr.WriteLine("START JSON ERRORS");            
        while (!foundEnd && !WScript.StdIn.AtEndOfStream) {
            if(currentInterationCount++ > maxIterationCount) {
                //throw {description : 'Maximum number of iterations exceeded'};
                WScript.Quit(1);
            }
        
            var line = WScript.StdIn.ReadLine();
            WScript.StdErr.WriteLine('line: ' + (line));
            if(!line || line.length <= 0 ) {
                WScript.StdErr.WriteLine('quitting with code 2');
                WScript.Quit(-2);
                
            }
            
            
            if (line === startDelim ) {
                foundStart = true;
                //TODO: Remove this
                WScript.StdErr.WriteLine('FOUND START');
            }
            else if (line === endDelim) {
                foundEnd = true;
                WScript.StdErr.WriteLine('FOUND END');
            }
            else if(foundStart) {
                strRead += line;
                strRead += '\n';
            }
        }
        
        
        
        
        WScript.StdErr.WriteLine(strRead);
        WScript.StdErr.WriteLine("END JSON ERRORS");

        //exit code here
        WScript.Quit(1);
    }
    catch(error) {
        WScript.Quit(-1);
    }
/*
	var jscriptToVerify = 'function updateProformaWarningVisible(checkBox,dropDownId,labelId){}\nfunction updateProformaWarningVisible(checkBox,dropDownId,labelId){}\n';
//	WScript.StdOut.WriteLine('****************************************');
	if (!JSLINT(jscriptToVerify, {passfail: false})) {	
		WScript.StdErr.WriteLine("START JSON ERRORS");
		//var jsonString = YAHOO.lang.JSON.stringify(JSLINT.errors);
		var jsonString = getErrorDetailsAsJson();
		WScript.StdErr.WriteLine(jsonString);
		WScript.StdErr.WriteLine("END JSON ERRORS");
		
		WScript.Quit(1);
	}
*/
})();
// SIG // Begin signature block
// SIG // MIIXWAYJKoZIhvcNAQcCoIIXSTCCF0UCAQExCzAJBgUr
// SIG // DgMCGgUAMGcGCisGAQQBgjcCAQSgWTBXMDIGCisGAQQB
// SIG // gjcCAR4wJAIBAQQQEODJBs441BGiowAQS9NQkAIBAAIB
// SIG // AAIBAAIBAAIBADAhMAkGBSsOAwIaBQAEFIIsHy6jOnJO
// SIG // jmYvO4voamkShNKioIISMTCCBGAwggNMoAMCAQICCi6r
// SIG // EdxQ/1ydy8AwCQYFKw4DAh0FADBwMSswKQYDVQQLEyJD
// SIG // b3B5cmlnaHQgKGMpIDE5OTcgTWljcm9zb2Z0IENvcnAu
// SIG // MR4wHAYDVQQLExVNaWNyb3NvZnQgQ29ycG9yYXRpb24x
// SIG // ITAfBgNVBAMTGE1pY3Jvc29mdCBSb290IEF1dGhvcml0
// SIG // eTAeFw0wNzA4MjIyMjMxMDJaFw0xMjA4MjUwNzAwMDBa
// SIG // MHkxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5n
// SIG // dG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVN
// SIG // aWNyb3NvZnQgQ29ycG9yYXRpb24xIzAhBgNVBAMTGk1p
// SIG // Y3Jvc29mdCBDb2RlIFNpZ25pbmcgUENBMIIBIjANBgkq
// SIG // hkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAt3l91l2zRTmo
// SIG // NKwx2vklNUl3wPsfnsdFce/RRujUjMNrTFJi9JkCw03Y
// SIG // SWwvJD5lv84jtwtIt3913UW9qo8OUMUlK/Kg5w0jH9FB
// SIG // JPpimc8ZRaWTSh+ZzbMvIsNKLXxv2RUeO4w5EDndvSn0
// SIG // ZjstATL//idIprVsAYec+7qyY3+C+VyggYSFjrDyuJSj
// SIG // zzimUIUXJ4dO3TD2AD30xvk9gb6G7Ww5py409rQurwp9
// SIG // YpF4ZpyYcw2Gr/LE8yC5TxKNY8ss2TJFGe67SpY7UFMY
// SIG // zmZReaqth8hWPp+CUIhuBbE1wXskvVJmPZlOzCt+M26E
// SIG // RwbRntBKhgJuhgCkwIffUwIDAQABo4H6MIH3MBMGA1Ud
// SIG // JQQMMAoGCCsGAQUFBwMDMIGiBgNVHQEEgZowgZeAEFvQ
// SIG // cO9pcp4jUX4Usk2O/8uhcjBwMSswKQYDVQQLEyJDb3B5
// SIG // cmlnaHQgKGMpIDE5OTcgTWljcm9zb2Z0IENvcnAuMR4w
// SIG // HAYDVQQLExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xITAf
// SIG // BgNVBAMTGE1pY3Jvc29mdCBSb290IEF1dGhvcml0eYIP
// SIG // AMEAizw8iBHRPvZj7N9AMA8GA1UdEwEB/wQFMAMBAf8w
// SIG // HQYDVR0OBBYEFMwdznYAcFuv8drETppRRC6jRGPwMAsG
// SIG // A1UdDwQEAwIBhjAJBgUrDgMCHQUAA4IBAQB7q65+Siby
// SIG // zrxOdKJYJ3QqdbOG/atMlHgATenK6xjcacUOonzzAkPG
// SIG // yofM+FPMwp+9Vm/wY0SpRADulsia1Ry4C58ZDZTX2h6t
// SIG // KX3v7aZzrI/eOY49mGq8OG3SiK8j/d/p1mkJkYi9/uEA
// SIG // uzTz93z5EBIuBesplpNCayhxtziP4AcNyV1ozb2AQWtm
// SIG // qLu3u440yvIDEHx69dLgQt97/uHhrP7239UNs3DWkuNP
// SIG // tjiifC3UPds0C2I3Ap+BaiOJ9lxjj7BauznXYIxVhBoz
// SIG // 9TuYoIIMol+Lsyy3oaXLq9ogtr8wGYUgFA0qvFL0QeBe
// SIG // MOOSKGmHwXDi86erzoBCcnYOMIIEejCCA2KgAwIBAgIK
// SIG // YQYngQAAAAAACDANBgkqhkiG9w0BAQUFADB5MQswCQYD
// SIG // VQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4G
// SIG // A1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0
// SIG // IENvcnBvcmF0aW9uMSMwIQYDVQQDExpNaWNyb3NvZnQg
// SIG // Q29kZSBTaWduaW5nIFBDQTAeFw0wODEwMjIyMTI0NTVa
// SIG // Fw0xMDAxMjIyMTM0NTVaMIGDMQswCQYDVQQGEwJVUzET
// SIG // MBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVk
// SIG // bW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0
// SIG // aW9uMQ0wCwYDVQQLEwRNT1BSMR4wHAYDVQQDExVNaWNy
// SIG // b3NvZnQgQ29ycG9yYXRpb24wggEiMA0GCSqGSIb3DQEB
// SIG // AQUAA4IBDwAwggEKAoIBAQC9crSJ5xyfhcd0uGBcAzY9
// SIG // nP2ZepopRiKwp4dT7e5GOsdbBQtXqLfKBczTTHdHcIWz
// SIG // 5cvfZ+ej/XQnk2ef14oDRDDG98m6yTodCFZETxcIDfm0
// SIG // GWiqJBz7BVeF6cVOByE3p+vOLC+2Qs0hBafW5tMoV8cb
// SIG // es4pNgfNnlXMu/Ei66gjpA0pwvvQw1o+Yz3HLEkLe3mF
// SIG // 8Ijvcb1DWuOjsw3zVfsl4OIg0+eaXpSlMy0of1cbVWoM
// SIG // MkTvZmxv8Dic7wKtmqHdmAcQDjwYaeJ5TkYU4LmM0HVt
// SIG // nKwAnC1C9VG4WvR4RYPpLnwru13NGWEorZRDCsVqQv+1
// SIG // Mq6kKSLeFujTAgMBAAGjgfgwgfUwEwYDVR0lBAwwCgYI
// SIG // KwYBBQUHAwMwHQYDVR0OBBYEFCPRcypMvfvlIfpxHpkV
// SIG // 0Rf5xKaKMA4GA1UdDwEB/wQEAwIHgDAfBgNVHSMEGDAW
// SIG // gBTMHc52AHBbr/HaxE6aUUQuo0Rj8DBEBgNVHR8EPTA7
// SIG // MDmgN6A1hjNodHRwOi8vY3JsLm1pY3Jvc29mdC5jb20v
// SIG // cGtpL2NybC9wcm9kdWN0cy9DU1BDQS5jcmwwSAYIKwYB
// SIG // BQUHAQEEPDA6MDgGCCsGAQUFBzAChixodHRwOi8vd3d3
// SIG // Lm1pY3Jvc29mdC5jb20vcGtpL2NlcnRzL0NTUENBLmNy
// SIG // dDANBgkqhkiG9w0BAQUFAAOCAQEAQynPY71s43Ntw5nX
// SIG // bQyIO8ZIc3olziziN3udNJ+9I86+39hceRFrE1EgAWO5
// SIG // cvcI48Z9USoWKNTR55sqzxgN0hNxkSnsVr351sUNL69l
// SIG // LW1NRSlWcoRPP9JqHUFiqXlcjvDHd4rLAiguncecK+W5
// SIG // Kgnd7Jfi5XqNXhCIU6HdYE93mHFgqFs5kdOrEh8F6cNF
// SIG // qdPCUbmvuNz8BoQA9HSj2//MHaAjBQfkJzXCl5AZqoJg
// SIG // J+j7hCse0QTLjs+CDdeoTUNAddLe3XfvilxrD4dkj7S6
// SIG // t7qrZ1QhRapKaOdUXosUXGd47JBcAxCRCJ0kIJfo3wAR
// SIG // cKn5snJwt67iwp8WAjCCBJ0wggOFoAMCAQICEGoLmU/A
// SIG // ACWrEdtFH1h6Z6IwDQYJKoZIhvcNAQEFBQAwcDErMCkG
// SIG // A1UECxMiQ29weXJpZ2h0IChjKSAxOTk3IE1pY3Jvc29m
// SIG // dCBDb3JwLjEeMBwGA1UECxMVTWljcm9zb2Z0IENvcnBv
// SIG // cmF0aW9uMSEwHwYDVQQDExhNaWNyb3NvZnQgUm9vdCBB
// SIG // dXRob3JpdHkwHhcNMDYwOTE2MDEwNDQ3WhcNMTkwOTE1
// SIG // MDcwMDAwWjB5MQswCQYDVQQGEwJVUzETMBEGA1UECBMK
// SIG // V2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwG
// SIG // A1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSMwIQYD
// SIG // VQQDExpNaWNyb3NvZnQgVGltZXN0YW1waW5nIFBDQTCC
// SIG // ASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBANw3
// SIG // bvuvyEJKcRjIzkg+U8D6qxS6LDK7Ek9SyIPtPjPZSTGS
// SIG // KLaRZOAfUIS6wkvRfwX473W+i8eo1a5pcGZ4J2botrfv
// SIG // hbnN7qr9EqQLWSIpL89A2VYEG3a1bWRtSlTb3fHev5+D
// SIG // x4Dff0wCN5T1wJ4IVh5oR83ZwHZcL322JQS0VltqHGP/
// SIG // gHw87tUEJU05d3QHXcJc2IY3LHXJDuoeOQl8dv6dbG56
// SIG // 4Ow+j5eecQ5fKk8YYmAyntKDTisiXGhFi94vhBBQsvm1
// SIG // Go1s7iWbE/jLENeFDvSCdnM2xpV6osxgBuwFsIYzt/iU
// SIG // W4RBhFiFlG6wHyxIzG+cQ+Bq6H8mjmsCAwEAAaOCASgw
// SIG // ggEkMBMGA1UdJQQMMAoGCCsGAQUFBwMIMIGiBgNVHQEE
// SIG // gZowgZeAEFvQcO9pcp4jUX4Usk2O/8uhcjBwMSswKQYD
// SIG // VQQLEyJDb3B5cmlnaHQgKGMpIDE5OTcgTWljcm9zb2Z0
// SIG // IENvcnAuMR4wHAYDVQQLExVNaWNyb3NvZnQgQ29ycG9y
// SIG // YXRpb24xITAfBgNVBAMTGE1pY3Jvc29mdCBSb290IEF1
// SIG // dGhvcml0eYIPAMEAizw8iBHRPvZj7N9AMBAGCSsGAQQB
// SIG // gjcVAQQDAgEAMB0GA1UdDgQWBBRv6E4/l7k0q0uGj7yc
// SIG // 6qw7QUPG0DAZBgkrBgEEAYI3FAIEDB4KAFMAdQBiAEMA
// SIG // QTALBgNVHQ8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAN
// SIG // BgkqhkiG9w0BAQUFAAOCAQEAlE0RMcJ8ULsRjqFhBwEO
// SIG // jHBFje9zVL0/CQUt/7hRU4Uc7TmRt6NWC96Mtjsb0fus
// SIG // p8m3sVEhG28IaX5rA6IiRu1stG18IrhG04TzjQ++B4o2
// SIG // wet+6XBdRZ+S0szO3Y7A4b8qzXzsya4y1Ye5y2PENtEY
// SIG // Ib923juasxtzniGI2LS0ElSM9JzCZUqaKCacYIoPO8cT
// SIG // ZXhIu8+tgzpPsGJY3jDp6Tkd44ny2jmB+RMhjGSAYwYE
// SIG // lvKaAkMve0aIuv8C2WX5St7aA3STswVuDMyd3ChhfEjx
// SIG // F5wRITgCHIesBsWWMrjlQMZTPb2pid7oZjeN9CKWnMyw
// SIG // d1RROtZyRLIj9jCCBKowggOSoAMCAQICCmEGlC0AAAAA
// SIG // AAkwDQYJKoZIhvcNAQEFBQAweTELMAkGA1UEBhMCVVMx
// SIG // EzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1Jl
// SIG // ZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3Jh
// SIG // dGlvbjEjMCEGA1UEAxMaTWljcm9zb2Z0IFRpbWVzdGFt
// SIG // cGluZyBQQ0EwHhcNMDgwNzI1MTkwMjE3WhcNMTMwNzI1
// SIG // MTkxMjE3WjCBszELMAkGA1UEBhMCVVMxEzARBgNVBAgT
// SIG // Cldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAc
// SIG // BgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjENMAsG
// SIG // A1UECxMETU9QUjEnMCUGA1UECxMebkNpcGhlciBEU0Ug
// SIG // RVNOOjdBODItNjg4QS05RjkyMSUwIwYDVQQDExxNaWNy
// SIG // b3NvZnQgVGltZS1TdGFtcCBTZXJ2aWNlMIIBIjANBgkq
// SIG // hkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAlYEKIEIYUXrZ
// SIG // le2b/dyH0fsOjxPqqjcoEnb+TVCrdpcqk0fgqVZpAuWU
// SIG // fk2F239x73UA27tDbPtvrHHwK9F8ks6UF52hxbr5937d
// SIG // YeEtMB6cJi12P+ZGlo6u2Ik32Mzv889bw/xo4PJkj5vo
// SIG // wxL5o76E/NaLzgU9vQF2UCcD+IS3FoaNYL5dKSw8z6X9
// SIG // mFo1HU8WwDjYHmE/PTazVhQVd5U7EPoAsJPiXTerJ7tj
// SIG // LEgUgVXjbOqpK5WNiA5+owCldyQHmCpwA7gqJJCa3sWi
// SIG // Iku/TFkGd1RyQ7A+ZN2ThAhYtv7ph0kJNrOz+DOpfkyi
// SIG // eX8yWSkOnrX14DyeP+xGOwIDAQABo4H4MIH1MB0GA1Ud
// SIG // DgQWBBQolYi/Ajvr2pS6fUYP+sv0fp3/0TAfBgNVHSME
// SIG // GDAWgBRv6E4/l7k0q0uGj7yc6qw7QUPG0DBEBgNVHR8E
// SIG // PTA7MDmgN6A1hjNodHRwOi8vY3JsLm1pY3Jvc29mdC5j
// SIG // b20vcGtpL2NybC9wcm9kdWN0cy90c3BjYS5jcmwwSAYI
// SIG // KwYBBQUHAQEEPDA6MDgGCCsGAQUFBzAChixodHRwOi8v
// SIG // d3d3Lm1pY3Jvc29mdC5jb20vcGtpL2NlcnRzL3RzcGNh
// SIG // LmNydDATBgNVHSUEDDAKBggrBgEFBQcDCDAOBgNVHQ8B
// SIG // Af8EBAMCBsAwDQYJKoZIhvcNAQEFBQADggEBAADurPzi
// SIG // 0ohmyinjWrnNAIJ+F1zFJFkSu6j3a9eH/o3LtXYfGyL2
// SIG // 9+HKtLlBARo3rUg3lnD6zDOnKIy4C7Z0Eyi3s3XhKgni
// SIG // i0/fmD+XtzQSgeoQ3R3cumTPTlA7TIr9Gd0lrtWWh+pL
// SIG // xOXw+UEXXQHrV4h9dnrlb/6HIKyTnIyav18aoBUwJOCi
// SIG // fmGRHSkpw0mQOkODie7e1YPdTyw1O+dBQQGqAAwL8tZJ
// SIG // G85CjXuw8y2NXSnhvo1/kRV2tGD7FCeqbxJjQihYOoo7
// SIG // i0Dkt8XMklccRlZrj8uSTVYFAMr4MEBFTt8ZiL31EPDd
// SIG // Gt8oHrRR8nfgJuO7CYES3B460EUxggSTMIIEjwIBATCB
// SIG // hzB5MQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2FzaGlu
// SIG // Z3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UEChMV
// SIG // TWljcm9zb2Z0IENvcnBvcmF0aW9uMSMwIQYDVQQDExpN
// SIG // aWNyb3NvZnQgQ29kZSBTaWduaW5nIFBDQQIKYQYngQAA
// SIG // AAAACDAJBgUrDgMCGgUAoIG+MBkGCSqGSIb3DQEJAzEM
// SIG // BgorBgEEAYI3AgEEMBwGCisGAQQBgjcCAQsxDjAMBgor
// SIG // BgEEAYI3AgEVMCMGCSqGSIb3DQEJBDEWBBQTsL8fBVyk
// SIG // nzHTCOWInhnslhI9tTBeBgorBgEEAYI3AgEMMVAwTqAm
// SIG // gCQATQBpAGMAcgBvAHMAbwBmAHQAIABMAGUAYQByAG4A
// SIG // aQBuAGehJIAiaHR0cDovL3d3dy5taWNyb3NvZnQuY29t
// SIG // L2xlYXJuaW5nIDANBgkqhkiG9w0BAQEFAASCAQCup+E0
// SIG // Z0b6i+yFXjd7AXkvlt+RpJ6xSp2WGHnBwzGlaPwsYh3e
// SIG // n6ILPBk9cCCo+Zzh/zQEDf/C7FzGXqlVHExN509zfL8a
// SIG // Z74uc3ddJCyDVHtwZF9vXRrTQBPVwU5X9e8fs9kbkspR
// SIG // qfK6k0GvgZhoEmZrH0v4/9sfgIl1XiiPvnD5S7Jidirk
// SIG // uni8/JGnExug37nJ4DbaTkP5Bc6vFVc+auQdqCMXIb0O
// SIG // YLR/lgZKQ/oegd8WfMXlruGjXot8LBFX2XbDGRX5Wx48
// SIG // karZ6wyoUOkaaUhnYwIQdY4AFhLK/7tIVaGEeAfb2gMP
// SIG // RSsggmefb7WCEBz3vqpGAtvdrCbooYICHzCCAhsGCSqG
// SIG // SIb3DQEJBjGCAgwwggIIAgEBMIGHMHkxCzAJBgNVBAYT
// SIG // AlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQH
// SIG // EwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29y
// SIG // cG9yYXRpb24xIzAhBgNVBAMTGk1pY3Jvc29mdCBUaW1l
// SIG // c3RhbXBpbmcgUENBAgphBpQtAAAAAAAJMAcGBSsOAwIa
// SIG // oF0wGAYJKoZIhvcNAQkDMQsGCSqGSIb3DQEHATAcBgkq
// SIG // hkiG9w0BCQUxDxcNMDgxMjA5MjAwMjIwWjAjBgkqhkiG
// SIG // 9w0BCQQxFgQU7jLosiIn//vtc3LLLDa4P4yDg3kwDQYJ
// SIG // KoZIhvcNAQEFBQAEggEAZIQ9dzGLdTw+xdWHVKNcJ5sw
// SIG // pJDJdBwfxKlvQYoUL7iHN+oGx62quKlxfusvQYsQI47n
// SIG // uFCjZhYNjtg7qD8qpzKAWwbFKEWgJgNIMfMliqlin6fw
// SIG // VDcgLQEuz2jyj3bsdU08BRRBXibrMVut3Uku3GIJNCaC
// SIG // 3IRkDNikQ2eYKGEkOknVPHKHr8cCqbm4OZ3DZ3hzLoTW
// SIG // Q53ngavBeZt7OBal93QKu040R+bHioLrjUrecsGbbTi4
// SIG // xgYHA/6DWkUB0H/NlUCKLabcZI0m7FOwpeeMFdHcyl+X
// SIG // NAzzfpQcmsULTBW/Em+saXoA4u6JaHEyXm2rbct5BXbi
// SIG // 2Tn1Cfihrw==
// SIG // End signature block
