echo a > b
echo hello >> b
echo c > d
echo hello >> d
mkdir e
echo hello > e/f

find . b | xargs grep hello
find . d | xargs grep hello
find e f | xargs grep hello