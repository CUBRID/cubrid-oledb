
var lib = eval(new ActiveXObject("Scripting.FileSystemObject").OpenTextFile("inc.js", 1).ReadAll());
var props = conn.Properties;
var i;

for (i = 0; i < props.Count; i++) {
	console.log(props.Item(i).Name + "=" + props.Item(i).Value);
}
