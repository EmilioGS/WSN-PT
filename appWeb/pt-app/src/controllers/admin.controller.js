//Import express
const express = require('express');
const app = express(); //create an express object called app

//Import obj readUser which contains the user model where we make the database connection
const readUser = require('../models/user.model'); 

module.exports={
    //Function that gets all users in table Usuario
    getAllUsers:(req,res)=>{
        readUser.query('SELECT * FROM Usuario', (req,result)=>{
            res.send(result);
        });
        
    },
    //Authentication function
    authLogin:(req,res)=>{
        console.log("Validar inicio de sesion")
        //Save in constants both email and psw form the request body
        const userEmail = req.body.email;
        const userPsw = req.body.contrasenia;
        console.log("usuario: "+userEmail);
        console.log("contraseña: "+userPsw);
        //Get info from the DB of the user which email=userEmail from request body
        readUser.query('SELECT * FROM Usuario where correo=?', [userEmail], (req,result)=>{
            //All the info from user is saved in result[0]
            //Compare if the psw got from DB is the same as the one got from the request body
            if (result[0].contrasenia==userPsw){ //success login
                console.log("Bienvenido, ingreso exitoso")
                //render the dashboardView.pug
                res.render('dashboardView'); 
            }else //failed login
                console.log("Contraseña incorrecta");
        });
    },
    //Function that shows all actions admin exclusive
    adminActions:(req,res)=>{
        const list = [];//create an empty array 
        //Get all users from DB
        readUser.query('SELECT * FROM Usuario ', (req,result)=>{
        //Fill the array with all the users info 
        for (let i = 0; i < result.length; i++) {
            //One user => one element of the array (i)
            list[i] = {
                // var in js array : var in result from DB query
                idUser: result[i].idUsuario,
                name: result[i].nombre,
                lastName: result[i].apellido,
                email: result[i].email,
                adminFlag: result[i].bandera_administrador,
                node: result[i].nodo,
                password: result[i].contrasenia
            };
        } 
        //Render adminView.pug; {var-in-Pug:var-in-Js}
        res.render('adminView', { userList: list }); 
        });
        
    },
    //Function that adds a user in the DB
    addUser: (req, res) => {
        //console.log(req.body);
        //Create a constant that contains the info in the body of the request 
        const { name, lastName, email, adminFlag, node, password } = req.body;
        //Insert query into table Usuario
        readUser.query('INSERT INTO Usuario (nombre, apellido, correo, bandera_administrador, nodo, contrasenia) VALUES (?, ?, ?, ?, ?, ?);', [name, lastName, email, adminFlag, node, password], (err, result) => {
            if (err) {
                console.error(err);
                res.status(500).send("Error al agregar usuario");
                return;
            }else{
                readUser.query('SET @idUsuario = LAST_INSERT_ID();');
                readUser.query('INSERT INTO Usuario_tiene_Nodo (Usuario_idUsuario, Nodo_idNodo) VALUES (@idUsuario,?)', [node], (err, result) => {
                    if (err) {
                        console.error(err);
                        res.status(500).send("Error al agregar usuario");
                        return;
                    }
                })
            }
            res.redirect('/');
        });
    },
    //Function that updates a user in DB
    updateUser: (req, res) => {
        //console.log(req.body);
        //Create a constant that contains the info in the body of the request
        const { idUpdate, name, lastName, email, adminFlag, node } = req.body;

        //Query to get the info of the user that idUsuario=idUpdate from the request
        readUser.query('SELECT * FROM Usuario WHERE idUsuario = ?', [idUpdate], (req, resultado) => {
            //console.log(resultado);
            //console.log(resultado[0].contrasenia);

            //Create a const that contains the password got from the DB query
            const contrasenia = resultado[0].contrasenia;       
            //Update query where (nombre, apellido, correo, bandera_administrador, nodo and idUsuario are from body request); and (contrasenia is from the select query in DB)
            readUser.query('UPDATE Usuario SET nombre = ?, apellido = ?, correo = ?, bandera_administrador = ?, nodo = ?, contrasenia = ? WHERE idUsuario = ?', [name, lastName, email, adminFlag, node, contrasenia, idUpdate], (err, result) => {
                if (err) {
                    console.error(err);
                    res.status(500).send("Error al actualizar usuario");
                    return;
                }
                res.redirect('/adminActions');
            });
        });
    },
    //Function that deletes a user from DB
    deleteUser: (req, res) => {
        //console.log(req.body);
        //Create a const that contains the id of the user to delete 
        const idDelete = req.body.idDelete;
        //Delete query in table User where idUsuario=idDelete from the body request
        readUser.query('DELETE FROM Usuario WHERE idUsuario = ?', [idDelete], (err, result) => {
            if (err) {
                console.error(err);
                res.status(500).send("Error al eliminar usuario");
                return;
            }
            res.redirect('user/adminActions');
        });
    }

}
