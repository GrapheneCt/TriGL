// Source code taken from https://github.com/bonigarcia/webgl-examples/blob/master/lighting/ambient_directional_light_cube.html

var liext = engine.loadPlugin("psp2/prx/liext.suprx");
var gl = engine.loadPlugin("psp2/prx/webgl.suprx");
var count = 0.0;
var ambientLight = new Float32Array([0.5, 0.1, 0.3]); // r, g, b
var lightColor = new Float32Array([1.0, 1.0, 1.0]); // r, g, b
var lightDirection = new Float32Array([2, -8, -8]); // x, y, z
var initOk = 0;

include( "js/app/gl-matrix-min.js" );

engine.onLoad = function(screen) {
	if(liext != null && gl != null) {
		console.log('Load OK');
		gl.createContext(gl.PSP2_WINDOW_1920X1088);
		
		// Init shaders
		var vs;
		var fs;
		
		engine.loadData('data/vs.txt', function (loadedText) { vs = loadedText; });
	    engine.loadData('data/fs.txt', function (loadedText) { fs = loadedText; init(vs, fs) });
	}
	else
		console.log('Load ERROR');
}

engine.onEnterFrame = function() {
    if (initOk) {
        // Hidden surface removal
        gl.enable(gl.DEPTH_TEST);

        // Draw Scene
        drawScene(gl);
    }
	
	gl.commit();
}

function init(vs, fs) {
	initShaders(gl, vs, fs);

	// Init vertex shader
	initVertexShader(gl);

	// Set clear canvas color
	gl.clearColor(0.0, 0.0, 0.0, 1.0);

	initOk = 1;
}

function drawScene(gl) {
	// Clear
	gl.clear(gl.COLOR_BUFFER_BIT);

	// Init projection
	initProjection(gl);

	// Rotate
	var mvMatrix = mat4.fromRotation(mat4.create(), count, [0.0, 1.0, 0.5]);
	var uMvMatrix = gl.getUniformLocation(gl.program, "u_mvMatrix");
	gl.uniformMatrix4fv(uMvMatrix, false, mvMatrix);

	// Draw
	gl.drawElements(gl.TRIANGLES, 6 * 2 * 3, gl.UNSIGNED_BYTE, 0);

	count += 0.01;
}

function initVertexShader(gl) {
	//    v6----- v5
	//   /|      /|
	//  v1------v0|
	//  | |     | |
	//  | |v7---|-|v4
	//  |/      |/
	//  v2------v3
	var vertices = new Float32Array([ // Coordinates
	0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5, // v0-v1-v2-v3 front
	0.5, 0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, // v0-v3-v4-v5 right
	0.5, 0.5, 0.5, 0.5, 0.5, -0.5, -0.5, 0.5, -0.5, -0.5, 0.5, 0.5, // v0-v5-v6-v1 up
	-0.5, 0.5, 0.5, -0.5, 0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, 0.5, // v1-v6-v7-v2 left
	-0.5, -0.5, -0.5, 0.5, -0.5, -0.5, 0.5, -0.5, 0.5, -0.5, -0.5, 0.5, // v7-v4-v3-v2 down
	0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5 // v4-v7-v6-v5 back
	]);


	var colors = new Float32Array([ // Colors
	1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, // v0-v1-v2-v3 front
	0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, // v0-v3-v4-v5 right
	0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, // v0-v5-v6-v1 up
	1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, // v1-v6-v7-v2 left
	0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, // v7-v4-v3-v2 down
	1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1 // v4-v7-v6-v5 back
	]);

	var normals = new Float32Array([ // Normal
	0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, // v0-v1-v2-v3 front
	1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, // v0-v3-v4-v5 right
	0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, // v0-v5-v6-v1 up
	-1.0, 0.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0, // v1-v6-v7-v2 left
	0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, // v7-v4-v3-v2 down
	0.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0, -1.0 // v4-v7-v6-v5 back
	]);

	gl.bindBuffer(gl.ARRAY_BUFFER, gl.createBuffer());
	gl.bufferData(gl.ARRAY_BUFFER, vertices, gl.STATIC_DRAW);

	var vertexPositionAttribute = gl.getAttribLocation(gl.program, "a_Position");
	gl.enableVertexAttribArray(vertexPositionAttribute);
	gl.vertexAttribPointer(vertexPositionAttribute, 3, gl.FLOAT, false, 0, 0);

	gl.bindBuffer(gl.ARRAY_BUFFER, gl.createBuffer());
	gl.bufferData(gl.ARRAY_BUFFER, colors, gl.STATIC_DRAW);

	var vertexColorAttribute = gl.getAttribLocation(gl.program, "a_Color");
	gl.enableVertexAttribArray(vertexColorAttribute);
	gl.vertexAttribPointer(vertexColorAttribute, 3, gl.FLOAT, false, 0, 0);

	gl.bindBuffer(gl.ARRAY_BUFFER, gl.createBuffer());
	gl.bufferData(gl.ARRAY_BUFFER, normals, gl.STATIC_DRAW);

	var vertexNormalAttribute = gl.getAttribLocation(gl.program, "a_Normal");
	gl.enableVertexAttribArray(vertexNormalAttribute);
	gl.vertexAttribPointer(vertexNormalAttribute, 3, gl.FLOAT, false, 0, 0);

	// Indices of the vertices
	var indices = new Uint8Array([0, 1, 2, 0, 2, 3, // front
	4, 5, 6, 4, 6, 7, // right
	8, 9, 10, 8, 10, 11, // up
	12, 13, 14, 12, 14, 15, // left
	16, 17, 18, 16, 18, 19, // down
	20, 21, 22, 20, 22, 23 // back
	]);

	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, gl.createBuffer());
	gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, indices, gl.STATIC_DRAW);
}

function initProjection(gl) {
	// Write u_pMatrix
	var ratio = gl.drawingBufferWidth / gl.drawingBufferHeight;
	var pMatrix = mat4.perspective(mat4.create(), 150, ratio, 0.1, 100);
	var pMatrixUniform = gl.getUniformLocation(gl.program, "u_pMatrix");
	gl.uniformMatrix4fv(pMatrixUniform, false, pMatrix);

	// Write u_vMatrix
	var vMatrix = mat4.lookAt(mat4.create(), [0, 0, -3], [0, 0, 0], [0, 1, 0]);
	var vMatrixUniform = gl.getUniformLocation(gl.program, "u_vMatrix");
	gl.uniformMatrix4fv(vMatrixUniform, false, vMatrix);

	// Write u_AmbientLight
	var ambientLightUniform = gl.getUniformLocation(gl.program, "u_AmbientLight");
	gl.uniform3fv(ambientLightUniform, ambientLight);

	// Write u_LightColor
	var lightColorUniform = gl.getUniformLocation(gl.program, "u_LightColor");
	gl.uniform3fv(lightColorUniform, lightColor);

	// Write u_LightDirection
	vec3.normalize(lightDirection, lightDirection);
	var lightDirectionUniform = gl.getUniformLocation(gl.program, "u_LightDirection");
	gl.uniform3fv(lightDirectionUniform, lightDirection);
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

	// Use program
	gl.useProgram(glProgram);
	gl.program = glProgram;
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