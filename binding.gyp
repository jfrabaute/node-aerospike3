{
  "targets": [
    {
      "target_name": "aerospike_cpp",
      "sources": [
        "src/aerospike.cc",
        "src/aerospike_client.cc",
        "src/aerospike_error.cc",
        "src/helper.cc"
      ],
      "link_settings": {
        "libraries": [
          "-laerospike"
        ]
      },
      "cflags": [
        "-std=c++0x"
      ],
    }
  ]
}
