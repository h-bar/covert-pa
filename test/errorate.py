from jiwer import wer

gt = open("../test/file.txt","r").read()
out = open("../test/out.txt", "r").read()

print(wer(gt, out))