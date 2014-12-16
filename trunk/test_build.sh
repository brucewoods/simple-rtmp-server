#!/bin/bash
rm -rf objs/ && ./configure --disable-all --with-ssl --with-http-callback && make
