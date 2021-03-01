const makeDefines = require('../defines.js');

const variant = process.argv[2];

Object.entries(makeDefines(variant)).forEach(([k, v]) =>
	console.log(
		`: |> echo "#define ${k} ${v}" > %o |> src/defs/${k}.h $(ROOT)/<gen_headers>`,
	),
);
