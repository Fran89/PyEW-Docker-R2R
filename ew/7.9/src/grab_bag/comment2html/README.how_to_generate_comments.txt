To generate html comments from the source tree on gldrocky, do
the following:

0.1) Make sure the $(WEB_DOC_DIR) environment variable is set:
gldrocky% setenv WEB_DOC_DIR /home/davidk/web

0.2) Build the source tree
(From earthworm/working/src/oracle)
gldrocky% make

1) Change to the earthworm working directory
gldrocky% cd /home/earthworm/working

2) Run the make_comment_filelist script
gldrocky% make_comment_filelist

3) Edit the comment_files file, and remove any
filenames that don't belong: such as schema-062101/*,
ewdb_ora_api.h-old and ./src/grab_bag/comment2html/comment_formats.txt
gldrocky% vi comment_files

4) Run comment2html to generate the comments directory tree.
(The directory you supply with the -O parameter should be the
same one as the $(WEB_DOC_DIR) environment variable that you
used to compile the source tree with. )
gldrocky% comment2html -O /home/davidk/web --INCLUDE comment_files


