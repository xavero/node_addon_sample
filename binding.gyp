{
  "targets": [
    {
		"target_name": "addon",
		"sources": [ "eventEmitter.cpp" ],
		"include_dirs": [
			"<!(node -e \"require('nan')\")",
		],
    }
  ]
}
