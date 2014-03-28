#!/bin/sh

GIT_URL=`git remote show origin | awk '/Push  URL/ { print $NF }'`
DEPLOY_DIR=/tmp/deploy.$$
mkdir ${DEPLOY_DIR}
(cd ${DEPLOY_DIR} ; \
    git clone ${GIT_URL} . \
 && git checkout gh-pages \
 && git rm -rf .
)
cp -r _build/html/* ${DEPLOY_DIR}
touch ${DEPLOY_DIR}/.nojekyll
(cd ${DEPLOY_DIR} ; \
    git add .nojekyll *  \
    && git commit -m "deploy"  \
    && git push)

rm -rf ${DEPLOY_DIR}
