echo $1
echo $2

data="{\"address\": \"$1\", \"content\": \"$2\"}"

echo $data

curl -H "Content-Type: application/json" -d "$data" 'http://api.stat.ucgc.local:8080/api/v2/util/message/sms' 
