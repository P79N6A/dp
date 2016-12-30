for file in `ls proto`
do
    protoc -I=proto/ --python_out=py_proto/ proto/${file}
done
