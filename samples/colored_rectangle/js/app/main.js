// Source code taken from https://www.tutorialspoint.com/webgl/webgl_drawing_a_triangle.htm

var liext = engine.loadPlugin("psp2/prx/liext.suprx");
var gl = engine.loadPlugin("psp2/prx/webgl.suprx");
var n;

engine.onLoad = function(screen) {
	if(liext != null && gl != null) {
		console.log('Load OK');
		gl.createContext(gl.PSP2_WINDOW_960X544);
		
		// Init shaders
		var vs = 'attribute vec4 a_Position; attribute vec4 a_Color; varying highp vec4 v_Color; void main() { gl_Position = a_Position; v_Color = a_Color; }';
		var fs = 'varying highp vec4 v_Color; void main() { gl_FragColor = v_Color; }';
		if (!initShaders(gl, vs, fs)) {
			console.log('Failed to intialize shaders.');
			return;
		}

		// Clear canvas
		gl.clearColor(0.0, 0.0, 0.0, 1.0);

		// Init buffers
		n = initBuffers(gl);
		if (n < 0) {
			console.log('Failed to init buffers');
			return;
		}
	}
	else
		console.log('Load ERROR');
}

engine.onEnterFrame = function() {
	
	gl.clear(gl.COLOR_BUFFER_BIT);
	
	// Draw
	gl.drawElements(gl.TRIANGLES, n, gl.UNSIGNED_SHORT, 0);
	
	gl.commit();
	
}

function initBuffers(gl) {
	// Vertices
	var dim = 3;
	var vertices = new Float32Array([-0.6, -0.6, 0.0, // 0
		-0.6, 0.6, 0.0, // 1
		0.6, 0.6, 0.0, // 2
		0.6, -0.6, 0.0, // 3
	]);
	gl.bindBuffer(gl.ARRAY_BUFFER, gl.createBuffer());
	gl.bufferData(gl.ARRAY_BUFFER, vertices, gl.STATIC_DRAW);
	var vertexPositionAttribute = gl.getAttribLocation(gl.program, "a_Position");
	gl.enableVertexAttribArray(vertexPositionAttribute);
	gl.vertexAttribPointer(vertexPositionAttribute, dim, gl.FLOAT, false, 0, 0);

	// Colors
	var colors = new Float32Array([
		1.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 0.0, 1.0,
		1.0, 0.0, 1.0,
	]);
	gl.bindBuffer(gl.ARRAY_BUFFER, gl.createBuffer());
	gl.bufferData(gl.ARRAY_BUFFER, colors, gl.STATIC_DRAW);
	var vertexColorAttribute = gl.getAttribLocation(gl.program, "a_Color");
	gl.enableVertexAttribArray(vertexColorAttribute);
	gl.vertexAttribPointer(vertexColorAttribute, dim, gl.FLOAT, false, 0, 0);

	// Indices
	var indices = new Uint16Array([
		0, 1, 2,
		0, 2, 3,
	]);
	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, gl.createBuffer());
	gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, indices, gl.STATIC_DRAW);

	// Return number of vertices
	return indices.length;
}

function initShaders(gl, vs_source, fs_source) {
	// Compile shaders
	var vertexShader = makeShader(gl, vs_source, gl.VERTEX_SHADER);
	var fragmentShader = makeShader(gl, fs_source, gl.FRAGMENT_SHADER);

	// Create program
	var glProgram = gl.createProgram();

	// Attach and link shaders to the program
	gl.attachShader(glProgram, vertexShader);
	gl.attachShader(glProgram, fragmentShader);
	gl.linkProgram(glProgram);
	if (!gl.getProgramParameter(glProgram, gl.LINK_STATUS)) {
		console.log("Unable to initialize the shader program");
		return false;
	}

	// Use program
	gl.useProgram(glProgram);
	gl.program = glProgram;

	return true;
}

function makeShader(gl, src, type) {
	var shader = gl.createShader(type);
	gl.shaderSource(shader, src);
	gl.compileShader(shader);
	if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
		console.log("Error compiling shader: " + gl.getShaderInfoLog(shader));
		return;
	}
	return shader;
}

