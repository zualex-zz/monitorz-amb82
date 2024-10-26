const char INDEX_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<style>
	h1 {
		text-align: center;
	}

	p {
		text-align: center;
	}
</style>

<body>
	<h1 style="background-color:DodgerBlue;">Hello! AmebaD mini</h1>
	<p> Click <a href="https://www.amebaiot.com">here</a> if you like Ameba!</p>
	<img id="stream" src="" crossorigin>
	<script>
		var baseHost = document.location.origin;
		var streamUrl = baseHost + ':81';
		const view = document.getElementById('stream');
		view.src = streamUrl;
		// view.src = 'http://zzz.servebeer.com:9090'
	</script>
</body>

</html>
)=====";