<?php declare(strict_types = 1);

namespace FireBirdTests;

function test_max_len_table(string $table, int $MAX_LEN, int $CHAR_LEN)
{
    $t = init_tmp_db()->start_transaction();

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

function test_fields25(\FireBird\Transaction $t): void
{
    $t->query(file_get_contents(__DIR__."/001-FIELDS25.sql"));
    $t->commit_ret();

    $t->query("INSERT INTO FIELDS25 (ID) VALUES (1)");
    $q = $t->query("SELECT * FROM FIELDS25");

    for($i = 0; $i < $q->out_vars_count; $i++){
        $info = $q->get_var_info_out($i);
        printf("%s/%s/%d\n", $info->field, $info->type_str, $info->length);
    }
}

function test_fields30(\FireBird\Transaction $t): void
{
    $t->query(file_get_contents(__DIR__."/001-FIELDS30.sql"));
    $t->commit_ret();

    $t->query("INSERT INTO FIELDS30 (ID) VALUES (DEFAULT)");
    $q = $t->query("SELECT * FROM FIELDS30");
    for($i = 0; $i < $q->out_vars_count; $i++){
        $info = $q->get_var_info_out($i);
        printf("%s/%s/%d\n", $info->field, $info->type_str, $info->length);
    }
}

function test_fields40(\FireBird\Transaction $t): void
{
    $t->query(file_get_contents(__DIR__."/001-FIELDS40.sql"));
    $t->commit_ret();

    $t->query("INSERT INTO FIELDS40 (ID) VALUES (DEFAULT)");
    $q = $t->query("SELECT * FROM FIELDS40");
    for($i = 0; $i < $q->out_vars_count; $i++){
        $info = $q->get_var_info_out($i);
        printf("%s/%s/%d\n", $info->field, $info->type_str, $info->length);
    }
}
