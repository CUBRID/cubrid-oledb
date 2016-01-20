

(function () {

	this.conn = new ActiveXObject("ADODB.Connection");
	this.conn.Open("Provider=CUBRIDProvider; Data Source=demodb; Location=10.34.64.67; User ID=dba; Password=; Port=30000; Charset=utf-8");

	this.adOpenForwardOnly = 0,
	this.adOpenStatic = 3,

	this.adLockReadOnly = 1,
	this.adLockOptimistic = 3,

	this.adCmdText = 1,
	this.adCmdUnknown = 8,
	
	this.console = {
		log: function (str) {
			WScript.StdOut.WriteLine(str);
		},
		error: function (str) {
			WScript.StdErr.WriteLine(str);
		}
	};

})();
