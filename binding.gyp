{
  "targets": [
    {
      "target_name": "aerospike",
      "sources": [
        "src/aerospike.cc",
        "src/aerospike_client.cc",
        "src/aerospike_error.cc"
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
