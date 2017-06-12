<?php
    session_start();
    // remove all session variables because new login is asked
    session_unset();

    require 'functions.php'
    
    if(!isset($_POST['submit'])) {
        redirectWithError('Incorrect request');
    }else{
        $conn = dbConnect();
        if(!isset($_POST['email'] || !isset($_POST['newpsw']) || !isset($_POST['checkpsw'])
        || $_POST['email'] === '' || $_POST['newpsw'] === '' || $_POST['checkpsw'] === ''){
            redirectWithError('Incorrect request');
        }

        $email = $_POST['email'];
        $psw1 = $_POST['newpsw'];
        $psw2 = $_POST['checkpsw'];

        //sanitize strings
        $email = $conn->real_escape_string(htmlentities(trim($_POST[$email])));
        $psw1 = $conn->real_escape_string($_POST[$_psw1]);
        $psw2 = $conn->real_escape_string($_POST[$psw2]);

        if(!filter_var($email, FILTER_VALIDATE_EMAIL)){
            //email wrong format error
            redirectWithError('email in wrong format');
        }
        if($psw1 != $psw2){
            //passwords don't match error
            redirectWithError('passwords do not match');
        }
        $hashedpsw = hash('sha256', $psw1);
        
        signup($conn, $email, $hashedpsw);
    }
?>