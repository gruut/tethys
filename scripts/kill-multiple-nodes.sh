#!/bin/bash
ps | grep config | awk '{print $1}' | xargs kill -9
