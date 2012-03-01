my $INDENT_OPTS = "-nbad -nbap -nbbb -cs -pcs -prs -saf -sai -saw -sob -ss ";

sub transform () {
    system "indent $INDENT_OPTS $cfile";
    $changed_on_disk = 1;
    $delta_worked |= delta_test ();
    $exit_delta_pass = 1;
}

1;
