for file in `ls proto`
do
    protoc -I=proto/ --java_out=java_proto/ proto/${file}
done
