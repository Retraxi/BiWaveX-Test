from sequence_align.pairwise import alignment_score, needleman_wunsch

seq_a = ["G", "A", "T", "T", "A", "C", "A"]
seq_b = ["G", "C", "A", "T", "G", "C", "G"]

aligned_seq_a, aligned_seq_b = needleman_wunsch(
    seq_a,
    seq_b,
    match_score=0,
    mismatch_score=4,
    indel_score=6,
    gap="_",
)

score = alignment_score(
    aligned_seq_a,
    aligned_seq_b,
    match_score=0,
    mismatch_score=4,
    indel_score=6,
    gap="_",
)
print(score)
