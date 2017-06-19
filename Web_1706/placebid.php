<?php
    require 'functions.php';

    isUserAuthenticated(true);
    $conn = dbConnect();

    if(!isset($_POST['placebid'])) {    //placebid è il nome del button
        //redirectWithError('isset post register');
        $error = 'isset post placebid';
        header('Location: profile.php?error='.urlencode($error));
        die();
    }else{
        $conn = dbConnect();
        if(!isset($_POST['thr']) || $_POST['thr'] === ''){
            //redirectWithError('Isset in login non va');
            $error = 'isset in placebid e thr non va';
            header('Location: profile.php?error='.urlencode($error));
            die();
        }

        //sanitize strings (trim removes spaces before and after the string)
        $thr = $conn->real_escape_string(htmlentities(trim($_POST['thr'])));

        //sanitize thr. ALLOW_FRACTION allows fraction separator (.)
        if(!filter_var($thr, FILTER_SANITIZE_NUMBER_FLOAT, FILTER_FLAG_ALLOW_FRACTION)){
            $error = 'offer must be a number (!filter_var)';
            header('Location: profile.php?error='.urlencode($error));
            die();
        }
        /*if(!is_float($thr)){
            $error = 'offer must be a number (is_float = false)';
            header('Location: profile.php?error='.urlencode($error));
            die();
        }*/

        $rounded = round($thr, 2);  //round with precision 2

        if($rounded > 999999999){
            $rounded = 999999999.99;
            $error = 'Your offer has been rounded to 999999999.99€';
            header('Location: profile.php?error='.urlencode($error));
        }else if($rounded < 0.01){
            $error = 'bid cannot be negative and or smaller than 0.01€';
            header('Location: profile.php?error='.urlencode($error));
            die();
        }

        computeBid($conn, $rounded);
    }
?>
