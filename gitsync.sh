#!bin/bash 

BRANCH=$(git branch --show-current)

git push -u origin $BRANCH
git switch main
git merge $BRANCH
git push
git switch $BRANCH
