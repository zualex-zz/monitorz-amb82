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
	<h1 style="background-color:DodgerBlue;">Motionz!</h1>
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