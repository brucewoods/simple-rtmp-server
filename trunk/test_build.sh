#!/bin/bash
rm -rf objs/ && ./configure --disable-all --with-ssl --log-info && make
