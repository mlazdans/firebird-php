<?php declare(strict_types = 1);

namespace FireBirdTests;

function test_max_len_table(string $table, int $MAX_LEN, int $CHAR_LEN)
{
    $t = init_tmp_db_ibase()->start_transaction();

    $c = 0;

    $fields = [
        str_repeat("F", $MAX_LEN),
        str_repeat("🥰", $CHAR_LEN),
    ];
    $fields_str = join('" INTEGER,"', $fields);
    $create_sql = sprintf('CREATE TABLE "%s" ("%s" INTEGER)', $table, $fields_str);

    $t->query($create_sql);
    $t->commit_ret();

    $data = []; foreach($fields as $f)$data[$f] = ++$c;
    insert_into($t, $table, $data);
    print "Table:$table\n";
    dump_table_rows($t, $table);
}

function test_max_len(int $MAX_LEN, int $CHAR_LEN)
{
    var_dump($MAX_LEN);

    test_max_len_table(str_repeat('T', $MAX_LEN), $MAX_LEN, $CHAR_LEN);
    test_max_len_table(str_repeat('😂', $CHAR_LEN), $MAX_LEN, $CHAR_LEN);
}
