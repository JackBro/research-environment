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
	WScript.StdErr.WriteLine("START JSON ERRORS");
	    WScript.StdOut.WriteLine('starting test');
    WScript.StdErr.WriteLine("START JSON ERRORS");

    var stdInValue = WScript.StdIn.ReadAll();
    WScript.StdErr.WriteLine(stdInValue);


    WScript.StdErr.WriteLine("END JSON ERRORS");

    //exit code here
    WScript.Quit(1);

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
// SIG // MIIXSwYJKoZIhvcNAQcCoIIXPDCCFzgCAQExCzAJBgUr
// SIG // DgMCGgUAMGcGCisGAQQBgjcCAQSgWTBXMDIGCisGAQQB
// SIG // gjcCAR4wJAIBAQQQEODJBs441BGiowAQS9NQkAIBAAIB
// SIG // AAIBAAIBAAIBADAhMAkGBSsOAwIaBQAEFL6kPU0694AP
// SIG // rw0JlyMS885iCe5ZoIISJDCCBGAwggNMoAMCAQICCi6r
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
// SIG // cKn5snJwt67iwp8WAjCCBJ0wggOFoAMCAQICCmFHUroA
// SIG // AAAAAAQwDQYJKoZIhvcNAQEFBQAweTELMAkGA1UEBhMC
// SIG // VVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcT
// SIG // B1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jw
// SIG // b3JhdGlvbjEjMCEGA1UEAxMaTWljcm9zb2Z0IFRpbWVz
// SIG // dGFtcGluZyBQQ0EwHhcNMDYwOTE2MDE1MzAwWhcNMTEw
// SIG // OTE2MDIwMzAwWjCBpjELMAkGA1UEBhMCVVMxEzARBgNV
// SIG // BAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQx
// SIG // HjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEn
// SIG // MCUGA1UECxMebkNpcGhlciBEU0UgRVNOOkQ4QTktQ0ZD
// SIG // Qy01NzlDMScwJQYDVQQDEx5NaWNyb3NvZnQgVGltZXN0
// SIG // YW1waW5nIFNlcnZpY2UwggEiMA0GCSqGSIb3DQEBAQUA
// SIG // A4IBDwAwggEKAoIBAQCbbdyGUegyOzc6liWyz2/uYbVB
// SIG // 0hg7Wp14Z7r4H9kIVZKIfuNBU/rsKFT+tdr+cDuVJ0h+
// SIG // Q6AyLyaBSvICdnfIyan4oiFYfg29Adokxv5EEQU1OgGo
// SIG // 6lQKMyyH0n5Bs+gJ2bC+45klprwl7dfTjtv0t20bSQvm
// SIG // 08OHbu5GyX/zbevngx6oU0Y/yiR+5nzJLPt5FChFwE82
// SIG // a1Map4az5/zhwZ9RCdu8pbv+yocJ9rcyGb7hSlG8vHys
// SIG // LJVql3PqclehnIuG2Ju9S/wnM8FtMqzgaBjYbjouIkPR
// SIG // +Y/t8QABDWTAyaPdD/HI6VTKEf/ceCk+HaxYwNvfqtyu
// SIG // ZRvTnbxnAgMBAAGjgfgwgfUwHQYDVR0OBBYEFE8YiYrS
// SIG // ygB4xuxZDQ/9fMTBIoDeMB8GA1UdIwQYMBaAFG/oTj+X
// SIG // uTSrS4aPvJzqrDtBQ8bQMEQGA1UdHwQ9MDswOaA3oDWG
// SIG // M2h0dHA6Ly9jcmwubWljcm9zb2Z0LmNvbS9wa2kvY3Js
// SIG // L3Byb2R1Y3RzL3RzcGNhLmNybDBIBggrBgEFBQcBAQQ8
// SIG // MDowOAYIKwYBBQUHMAKGLGh0dHA6Ly93d3cubWljcm9z
// SIG // b2Z0LmNvbS9wa2kvY2VydHMvdHNwY2EuY3J0MBMGA1Ud
// SIG // JQQMMAoGCCsGAQUFBwMIMA4GA1UdDwEB/wQEAwIGwDAN
// SIG // BgkqhkiG9w0BAQUFAAOCAQEANyce9YxA4PZlJj5kxJC8
// SIG // PuNXhd1DDUCEZ76HqCra3LQ2IJiOM3wuX+BQe2Ex8xoT
// SIG // 3oS96mkcWHyzG5PhCCeBRbbUcMoUt1+6V+nUXtA7Q6q3
// SIG // P7baYYtxz9R91Xtuv7TKWjCR39oKDqM1nyVhTsAydCt6
// SIG // BpRyAKwYnUvlnivFOlSspGDYp/ebf9mpbe1Ea7rc4BL6
// SIG // 8K2HDJVjCjIeiU7MzH6nN6X+X9hn+kZL0W0dp33SvgL/
// SIG // 826C84d0xGnluXDMS2WjBzWpRJ6EfTlu/hQFvRpQIbU+
// SIG // n/N3HI/Cmp1X4Wl9aeiDzwJvKiK7NzM6cvrWMB2RrfZQ
// SIG // GusT3jrFt1zNszCCBJ0wggOFoAMCAQICEGoLmU/AACWr
// SIG // EdtFH1h6Z6IwDQYJKoZIhvcNAQEFBQAwcDErMCkGA1UE
// SIG // CxMiQ29weXJpZ2h0IChjKSAxOTk3IE1pY3Jvc29mdCBD
// SIG // b3JwLjEeMBwGA1UECxMVTWljcm9zb2Z0IENvcnBvcmF0
// SIG // aW9uMSEwHwYDVQQDExhNaWNyb3NvZnQgUm9vdCBBdXRo
// SIG // b3JpdHkwHhcNMDYwOTE2MDEwNDQ3WhcNMTkwOTE1MDcw
// SIG // MDAwWjB5MQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2Fz
// SIG // aGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UE
// SIG // ChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSMwIQYDVQQD
// SIG // ExpNaWNyb3NvZnQgVGltZXN0YW1waW5nIFBDQTCCASIw
// SIG // DQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBANw3bvuv
// SIG // yEJKcRjIzkg+U8D6qxS6LDK7Ek9SyIPtPjPZSTGSKLaR
// SIG // ZOAfUIS6wkvRfwX473W+i8eo1a5pcGZ4J2botrfvhbnN
// SIG // 7qr9EqQLWSIpL89A2VYEG3a1bWRtSlTb3fHev5+Dx4Df
// SIG // f0wCN5T1wJ4IVh5oR83ZwHZcL322JQS0VltqHGP/gHw8
// SIG // 7tUEJU05d3QHXcJc2IY3LHXJDuoeOQl8dv6dbG564Ow+
// SIG // j5eecQ5fKk8YYmAyntKDTisiXGhFi94vhBBQsvm1Go1s
// SIG // 7iWbE/jLENeFDvSCdnM2xpV6osxgBuwFsIYzt/iUW4RB
// SIG // hFiFlG6wHyxIzG+cQ+Bq6H8mjmsCAwEAAaOCASgwggEk
// SIG // MBMGA1UdJQQMMAoGCCsGAQUFBwMIMIGiBgNVHQEEgZow
// SIG // gZeAEFvQcO9pcp4jUX4Usk2O/8uhcjBwMSswKQYDVQQL
// SIG // EyJDb3B5cmlnaHQgKGMpIDE5OTcgTWljcm9zb2Z0IENv
// SIG // cnAuMR4wHAYDVQQLExVNaWNyb3NvZnQgQ29ycG9yYXRp
// SIG // b24xITAfBgNVBAMTGE1pY3Jvc29mdCBSb290IEF1dGhv
// SIG // cml0eYIPAMEAizw8iBHRPvZj7N9AMBAGCSsGAQQBgjcV
// SIG // AQQDAgEAMB0GA1UdDgQWBBRv6E4/l7k0q0uGj7yc6qw7
// SIG // QUPG0DAZBgkrBgEEAYI3FAIEDB4KAFMAdQBiAEMAQTAL
// SIG // BgNVHQ8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zANBgkq
// SIG // hkiG9w0BAQUFAAOCAQEAlE0RMcJ8ULsRjqFhBwEOjHBF
// SIG // je9zVL0/CQUt/7hRU4Uc7TmRt6NWC96Mtjsb0fusp8m3
// SIG // sVEhG28IaX5rA6IiRu1stG18IrhG04TzjQ++B4o2wet+
// SIG // 6XBdRZ+S0szO3Y7A4b8qzXzsya4y1Ye5y2PENtEYIb92
// SIG // 3juasxtzniGI2LS0ElSM9JzCZUqaKCacYIoPO8cTZXhI
// SIG // u8+tgzpPsGJY3jDp6Tkd44ny2jmB+RMhjGSAYwYElvKa
// SIG // AkMve0aIuv8C2WX5St7aA3STswVuDMyd3ChhfEjxF5wR
// SIG // ITgCHIesBsWWMrjlQMZTPb2pid7oZjeN9CKWnMywd1RR
// SIG // OtZyRLIj9jGCBJMwggSPAgEBMIGHMHkxCzAJBgNVBAYT
// SIG // AlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQH
// SIG // EwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29y
// SIG // cG9yYXRpb24xIzAhBgNVBAMTGk1pY3Jvc29mdCBDb2Rl
// SIG // IFNpZ25pbmcgUENBAgphBieBAAAAAAAIMAkGBSsOAwIa
// SIG // BQCggb4wGQYJKoZIhvcNAQkDMQwGCisGAQQBgjcCAQQw
// SIG // HAYKKwYBBAGCNwIBCzEOMAwGCisGAQQBgjcCARUwIwYJ
// SIG // KoZIhvcNAQkEMRYEFKcxyTV7LJVDSo4zddTjmxC3DIri
// SIG // MF4GCisGAQQBgjcCAQwxUDBOoCaAJABNAGkAYwByAG8A
// SIG // cwBvAGYAdAAgAEwAZQBhAHIAbgBpAG4AZ6EkgCJodHRw
// SIG // Oi8vd3d3Lm1pY3Jvc29mdC5jb20vbGVhcm5pbmcgMA0G
// SIG // CSqGSIb3DQEBAQUABIIBALDxUhYwE5jHP1azHxr2gsBf
// SIG // waj+7dy9ax1lNZ10NXlFn69qJlbNHxU+i2kff340cabl
// SIG // bOGY3taUmkr9jYXBdevFLe8PWxbKuNOJIJ6G16RgCWZX
// SIG // nbOCPrgydhtbxjrpL3I2HqaR34MhfooV+V8zu9ufnNEe
// SIG // kKo7shGHZbHOPByTYCq7CpAY/prjk2FhhBNjNciFeUA6
// SIG // /5UlxQXzPWp9vi8ftyaV6NODXDpeBhEPGxIJaNpA65kB
// SIG // g3PD3COWA172soX8UR0jOmkC3bKJdYVDApjID3a0E7ep
// SIG // 8cxPRvERXbSp3dM1hTe2Zh8EFbvEuQz2rOZkXTnwpUYr
// SIG // q/yll+HJ/KahggIfMIICGwYJKoZIhvcNAQkGMYICDDCC
// SIG // AggCAQEwgYcweTELMAkGA1UEBhMCVVMxEzARBgNVBAgT
// SIG // Cldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAc
// SIG // BgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEjMCEG
// SIG // A1UEAxMaTWljcm9zb2Z0IFRpbWVzdGFtcGluZyBQQ0EC
// SIG // CmFHUroAAAAAAAQwBwYFKw4DAhqgXTAYBgkqhkiG9w0B
// SIG // CQMxCwYJKoZIhvcNAQcBMBwGCSqGSIb3DQEJBTEPFw0w
// SIG // ODEyMDkyMDAyMjBaMCMGCSqGSIb3DQEJBDEWBBSzS1Ld
// SIG // ZkaxHfUU0slVkYwswDpGVTANBgkqhkiG9w0BAQUFAASC
// SIG // AQAVn12HE1ymhpUiSMl2AjZcPzNf0v68T/hv16Pt4xNr
// SIG // H6RcSeCEUVkmgrZOvb87SFSJ1GeT4Y8WGsxEkRlKxOk6
// SIG // W624/WHKW1HXCMKGjYFmtwzGVJWAev/2sVDcBmyPZ/SJ
// SIG // IXrbtz78TdR8UeCdSKhM9Sw9mId9Ce0m3z47c2WU47zk
// SIG // DRQ39Q9Nmk0mHBUmggZzAv2QLaZBHVwYdlNTLzeNaNGs
// SIG // kqPzo5IgDRdySAOcBqw6RgUCGty0CCEPPMvdhhmSgXPi
// SIG // 2DgRkjMB2LYx83rMHykAeV2sanB0GLNTYb5pavg4ZBZj
// SIG // 0h4x8LwCEdUs+n7VGwDxBiGFuRrSBLSJlQ65
// SIG // End signature block
