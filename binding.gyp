{
  "targets": [
    {
      "target_name": "aerospike",
      "sources": [
        "src/aerospike.cc",
        "src/aerospike_client.cc"
      ],
      "link_settings": {
        "libraries": [
          "-laerospike"
        ]
      }
    }
  ]
}
