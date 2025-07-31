# dx - Enhance Developer Experience!
```
git init && git add . && git commit -m "feat: dx" && git branch -M main && git remote add origin https://github.com/manfromexistence/formatter-and-linter.git && git push -u origin main

find . -maxdepth 1 -mindepth 1 -exec du -sh {} + | sort -rh | sed 's/K/KB/; s/M/MB/; s|\./||'

find . -maxdepth 1 -mindepth 1 -exec du -sh {} + | sed 's/K/KB/; s/M/MB/; s|\./||'

find . -type d -name "tests" -exec rm -r {} +

find . -maxdepth 1 -mindepth 1 ! -name "cli" ! -name "src" ! -name "creates" ! -name "packages" -exec rm -rf {} +
```


Remove all comments from this rust and don't change anything for now!

```
gcc -O3 main.c -o main
./main
```