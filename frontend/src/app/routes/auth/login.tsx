import {useNavigate} from 'react-router';
import { Form } from "radix-ui";
import {paths} from '@/config/paths';
import {env} from "@/config/env.ts";
import "./styles.css";

const LoginRoute = () => {
    const navigate = useNavigate();
    const username: string = "bob"
    const password: string = "Password123!"
    const body = { username, password }
    const bodyString = JSON.stringify(body)

    return (
        <>
        <button onClick={async ()=>{
            console.log("Trying to log in with username:", username, "and password:", password );
            const response = await fetch(env.API_URL+'/api/users/login', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: bodyString,
                credentials: 'include',
            })

            if (response.ok) {
                console.log("Login successful, navigating to home page");
                navigate(paths.home.getHref());
            } else {
                const responseData = await response.json();
                console.error("Login failed:", responseData);
            }

        }}>test login</button>
            <Form.Root className="FormRoot">
                <Form.Field className="FormField" name="email">
                    <div
                        style={{
                            display: "flex",
                            alignItems: "baseline",
                            justifyContent: "space-between",
                        }}
                    >
                        <Form.Label className="FormLabel">Email</Form.Label>
                        <Form.Message className="FormMessage" match="valueMissing">
                            Please enter your email
                        </Form.Message>
                        <Form.Message className="FormMessage" match="typeMismatch">
                            Please provide a valid email
                        </Form.Message>
                    </div>
                    <Form.Control asChild>
                        <input className="Input" type="email" required />
                    </Form.Control>
                </Form.Field>
                <Form.Field className="FormField" name="question">
                    <div
                        style={{
                            display: "flex",
                            alignItems: "baseline",
                            justifyContent: "space-between",
                        }}
                    >
                        <Form.Label className="FormLabel">Question</Form.Label>
                        <Form.Message className="FormMessage" match="valueMissing">
                            Please enter a question
                        </Form.Message>
                    </div>
                    <Form.Control asChild>
                        <textarea className="Textarea" required />
                    </Form.Control>
                </Form.Field>
                <Form.Submit asChild>
                    <button className="Button" style={{ marginTop: 10 }}>
                        Post question
                    </button>
                </Form.Submit>
            </Form.Root>

        </>
    );
};

export default LoginRoute;
